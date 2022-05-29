/*
  Delocto Chess Engine
  Copyright (c) 2018-2021 Moritz Terink

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "board.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "hashkeys.hpp"

#include <algorithm>
#include <sstream>
#include <ctype.h>

constexpr unsigned CASTLE_MASK[SQUARE_COUNT] = {

    14, 15, 15, 12, 15, 15, 15, 13,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    11, 15, 15,  3, 15, 15, 15,  7

};

static const char PieceToChar[COLOR_COUNT][PIECETYPE_COUNT] = {
    { 'P', 'N', 'B', 'R', 'Q', 'K' },
    { 'p', 'n', 'b', 'r', 'q', 'k' }
};

// Remove all pieces from the board and reset the game state
void Board::clear() {

    // Clear the state and move lists
    states.clear();
    moves.clear();

    // Clear all bitboards
    bbColors[WHITE] = 0;
    bbColors[BLACK] = 0;
    bbColors[BOTH] = 0;

    for (Color c = WHITE; c < COLOR_COUNT; c++) {
        for (Piecetype pt = PAWN; pt < PIECETYPE_COUNT; pt++) {
            bbPieces[pt] = 0;
            pieceCounts[c][pt] = 0; // Reset the piece counters
        }
    }
    // Clear the piece type array
    pieceTypes.fill(PIECE_NONE);

    // Reset the game state
    state.enPassant = SQUARE_NONE;
    state.castlingRights = 0;
    state.fiftyMoves = 0;
    state.repetitionCount = 0;
    state.material[WHITE] = V(0, 0);
    state.material[BLACK] = V(0, 0);
    state.captured = PIECE_NONE;
    state.pst[WHITE] = V(0, 0);
    state.pst[BLACK] = V(0, 0);
    state.kingBlockers[WHITE] = 0;
    state.kingBlockers[BLACK] = 0;
    state.checkers = 0;

    // Reset the number of half moves played so far to 0
    ply = 0;

    stm = WHITE;

}

// Calculate position, pawn and material hash key
void Board::calc_keys() {

    state.hashKey = 0;
    state.pawnKey = 0;
    state.materialKey = 0;

    Bitboard occupied = bbColors[BOTH];

    while (occupied) {

        const Square sq    = pop_lsb(occupied);
        const Color color  = owner(sq);
        const Piecetype pt = pieceTypes[sq];

        // Update the position hash key with the piece type and position
        hash_piece(color, pt, sq);

        // If the piece is a pawn, update the pawn hash key
        if (pt == PAWN) {
            hash_pawn(color, sq);
        }

    }

    // Generate the material hash key
    for (Color c = WHITE; c < COLOR_COUNT; c++) {
        for (Piecetype pt = PAWN; pt < PIECETYPE_COUNT; pt++) {
            hash_material(c, pt);
        }
    }

    // Update the position hash key with en-passant square, castling rights and the color to move
    hash_enPassant();
    hash_castling();
    hash_turn();

}

// Update the pieces blocking king attacks and get all pieces attacking the king
void Board::update_check_info() {

    state.kingBlockers[WHITE] = get_slider_blockers(bbColors[BLACK], lsb_index(pieces(WHITE, KING)));
    state.kingBlockers[BLACK] = get_slider_blockers(bbColors[WHITE], lsb_index(pieces(BLACK, KING)));

    state.checkers = sq_attackers(!stm, lsb_index(pieces(stm, KING)), bbColors[BOTH]);

}

// Update the number of times this position has appeared during the current game already
// Used for detection of 3-fold repetitions
void Board::update_repetition_count() {

    state.repetitionCount = 0;

    for (int i = static_cast<int>(states.size()) - 2; i >= 0; i -= 2) {
        if (states[i].hashKey == state.hashKey) {
            state.repetitionCount++;
        }
    }

}

// Get a bitboard of all pieces blocking a sliding attack to a square
Bitboard Board::get_slider_blockers(const Bitboard sliders, const Square sq) const {

    // We first try to find all potential pinners by looking in all directions from the square
    Bitboard blockers = 0;
    Bitboard pinners  = ((BishopAttacks[sq] & (bbPieces[BISHOP] | bbPieces[QUEEN])) | (RookAttacks[sq] & (bbPieces[ROOK] | bbPieces[QUEEN]))) & sliders;
    Bitboard occupied = bbColors[BOTH] ^ pinners ^ SQUARES[sq];

    // We loop over all potential pinners until there are no more left
    while (pinners) {

        const Square psq   = pop_lsb(pinners);
        const Bitboard pin = RayTable[psq][sq] & occupied;

        // If there are more than two pieces on the ray, they are not pinned
        if (popcount(pin) == 1) {
            blockers |= pin;
        }

    }

    return blockers;

}

// Most Valuable Victim - Least Valuable Attacker
// The function returns a score given a move which captures a piece or is en-passant
// There are two arrays, MvvLvaVictim and MvvLvaAttacker, which are indexed by piece type
// The formula is simple: MvvLvaVictim value minus MvvLvaAttacker value
// The method encourages capturing major pieces with minor pieces/pawns
int Board::mvvlva(const Move move) const {

    // For en-passant moves, the captured piece is a pawn
    if (is_ep(move)) {
        return MvvLvaVictim[PAWN] - MvvLvaAttacker[PAWN];
    }

    int value = 0;

    // For promotions, we add the piece value to the balance
    if (is_promotion(move)) {
        value += MvvLvaVictim[prom_piecetype(move_type(move))];
    }

    // Subtract the value of the attacker from the value of the victim
    value += MvvLvaVictim[pieceTypes[to_sq(move)]] - MvvLvaAttacker[pieceTypes[from_sq(move)]];

    return value;

}

// Sets the current position to a given FEN string
void Board::set_fen(std::string fen) {

    // Clear the board
    clear();

    static std::string nums = "12345678";

    unsigned i;
    unsigned r = 0;
    unsigned f = 0;

    // We loop over the FEN string to obtain the positions of the pieces until we encounter a space character
    for (i = 0; fen[i] != ' '; i++) {

        // The current square index. Because of our board layout we start from the last square
        Square sq = 63 - square(f, r);

        // A slash signifies that the current rank has finished
        if (fen[i] == '/') {
            f = 0;
            r++;
        } else if (nums.find(fen[i]) != std::string::npos) {
            f += fen[i] - '0';
        } else {
            Color color;
            Piecetype type;
            switch (fen[i]) {

                case 'P': { color = WHITE; type = PAWN; break; }
                case 'N': { color = WHITE; type = KNIGHT; break; }
                case 'B': { color = WHITE; type = BISHOP; break; }
                case 'R': { color = WHITE; type = ROOK; break; }
                case 'Q': { color = WHITE; type = QUEEN; break; }
                case 'K': { color = WHITE; type = KING; break; }
                case 'p': { color = BLACK; type = PAWN; break; }
                case 'n': { color = BLACK; type = KNIGHT; break; }
                case 'b': { color = BLACK; type = BISHOP; break; }
                case 'r': { color = BLACK; type = ROOK; break; }
                case 'q': { color = BLACK; type = QUEEN; break; }
                case 'k': { color = BLACK; type = KING; break; }
                default: {
                    std::cerr << "Invalid character in fen at position " << i << ": \"" << fen[i] << "\"" << std::endl;
                    std::exit(EXIT_FAILURE);
                }

            }

            // Update the piece square table values and the material balances
            state.pst[color]      += PieceSquareTable[color][type][sq];
            state.material[color] += Material[type];

            // Update the bitboards
            bbColors[color] |= SQUARES[sq];
            bbColors[BOTH]  |= SQUARES[sq];
            bbPieces[type]  |= SQUARES[sq];

            pieceTypes[sq] = type;
            pieceCounts[color][type]++;

            f++;
        }

    }

    ++i;

    // Extract the color to move from the FEN string
    stm = (fen[i] == 'w') ? WHITE : BLACK;
    i += 2;

    // Scan over the next part of the FEN string. It contains the castling rights
    while (fen[i] != ' ') {

        switch (fen[i]) {

            case 'K': { state.set_castling_right(CASTLE_WHITE_SHORT); break; }
            case 'Q': { state.set_castling_right(CASTLE_WHITE_LONG); break; }
            case 'k': { state.set_castling_right(CASTLE_BLACK_SHORT); break; }
            case 'q': { state.set_castling_right(CASTLE_BLACK_LONG); break; }

        }

        ++i;

    }

    ++i;

    // The next part may either be a dash or a square name for the en-passant square
    state.enPassant = (fen[i] != '-') ? square(7 - (fen[i] - 'a'), fen[i + 1] - '1') : SQUARE_NONE;

    i += 2;

    if (state.enPassant != SQUARE_NONE) {
        i++;
    }

    // The final part of the FEN string is the number of half moves made since the last capture or pawn move
    // and the number of full moves played on the board so far
    if (fen.size() >= i && isdigit(fen[i])) {
        std::string cut = fen.substr(i);
        unsigned space = cut.find(" ");
        state.fiftyMoves = std::stoi(cut.substr(0, space));
        ply = (std::stoi(cut.substr(space)) - 1) * 2;
    }

    // Update checkers and king blockers and calculate the hash keys
    update_check_info();
    calc_keys();

}

// Get a FEN string from the current piece setup on the board
std::string Board::get_fen() const {

    std::string fen;
    unsigned emptySquaresCount = 0;

    for (int rank = RANK_8; rank >= RANK_1; rank--) {
        for (int file = FILE_A; file >= FILE_H; file--) {

            const Square sq = square(file, rank);

            if (is_sq_empty(sq)) {
                emptySquaresCount++;
                continue;
            }
            
            if (emptySquaresCount > 0) {
                fen += std::to_string(emptySquaresCount);
                emptySquaresCount = 0;
            }

            fen += PieceToChar[owner(sq)][piecetype(sq)];

        }

        if (emptySquaresCount > 0) {
            fen += std::to_string(emptySquaresCount);
            emptySquaresCount = 0;
        }

        if (rank > RANK_1) {
            fen += '/';
        }

    }

    // Current turn
    fen += stm == WHITE ? " w " : " b ";

    // Castling rights
    if (state.castlingRights == CASTLE_NONE) {
        fen += '-';
    } else {
        if (state.castlingRights & CASTLE_WHITE_SHORT) {
            fen += 'K';
        }
        if (state.castlingRights & CASTLE_WHITE_LONG) {
            fen += 'Q';
        }
        if (state.castlingRights & CASTLE_BLACK_SHORT) {
            fen += 'k';
        }
        if (state.castlingRights & CASTLE_BLACK_LONG) {
            fen += 'q';
        }
    }

    Square epSq = enpassant_square();

    // En-passant square
    fen += ' ' + (epSq != SQUARE_NONE ? SQUARE_NAMES[epSq] : "-") + ' ';

    // Halfmove & fullmove number
    fen += std::to_string(state.fiftyMoves) + ' ' + std::to_string(ply / 2 + (ply % 2 == 0));

    return fen;

}

// Convert the current position to a string for debuging purposals
std::string Board::to_string() const {

    std::stringstream ss;

    char labels[2][6] = { { 'P', 'N', 'B', 'R', 'Q', 'K' }, { 'p', 'n', 'b', 'r', 'q', 'k' } };
    unsigned r  = 0;
    unsigned sq = 0;

    while (sq != SQUARE_NONE) {
        if (rank(sq) != r) {
            ss << std::endl;
            r++;
        }

        if (pieceTypes[sq] != PIECE_NONE) {
            ss << labels[owner(sq)][pieceTypes[sq]] << ' ';
        } else {
            ss << ". ";
        }
        sq++;
    }

    ss << std::endl;

    return ss.str();

}

// Adds a piece to the board given a square and a color
void Board::add_piece(const Color color, const Piecetype pt, const Square sq) {

    hash_material(color, pt);

    // Add the piece to the corresponding bitboards and increase the piece counter
    bbColors[color] |= SQUARES[sq];
    bbPieces[pt]    |= SQUARES[sq];
    pieceTypes[sq]  = pt;
    pieceCounts[color][pt]++;

    // Update the material balance and the piece square table value
    state.material[color] += Material[pt];
    state.pst[color]      += PieceSquareTable[color][pt][sq];

    // Update the hash keys
    hash_material(color, pt);
    hash_piece(color, pt, sq);
    if (pt == PAWN) {
        hash_pawn(color, sq);
    }

}

// Removes a piece from the board
void Board::remove_piece(const Square sq) {

    const Color color  = owner(sq);
    const Piecetype pt = pieceTypes[sq];

    // Update the material hash key
    hash_material(color, pt);

    // Remove the piece from the bitboards and update the piece counts
    bbColors[color] ^= SQUARES[sq];
    bbPieces[pt]    ^= SQUARES[sq];
    pieceTypes[sq]   = PIECE_NONE;
    pieceCounts[color][pt]--;

    // Remove the piece value from the material balance and update the piece square table value
    state.material[color] -= Material[pt];
    state.pst[color]      -= PieceSquareTable[color][pt][sq];

    // Update the hash keys and also the pawn hash key if the removed piece was a pawn
    hash_material(color, pt);
    hash_piece(color, pt, sq);
    if (pt == PAWN) {
        hash_pawn(color, sq);
    }

}

// Move a piece on the board from one square to another
void Board::move_piece(const Square fromSq, const Square toSq) {

    const Color color  = owner(fromSq);
    const Piecetype pt = pieceTypes[fromSq];

    // First, remove the piece on the corresponding bitboards.
    // Then, add it on the new square
    bbColors[color] ^= SQUARES[fromSq];
    bbPieces[pt]    ^= SQUARES[fromSq];
    bbColors[color] |= SQUARES[toSq];
    bbPieces[pt]    |= SQUARES[toSq];

    // Update the piece types array to match the new position of the piece
    pieceTypes[fromSq] = PIECE_NONE;
    pieceTypes[toSq] = pt;

    // Update the piece square table values
    state.pst[color] -= PieceSquareTable[color][pt][fromSq];
    state.pst[color] += PieceSquareTable[color][pt][toSq];

    // Update the position hash key
    hash_piece(stm, pt, fromSq);
    hash_piece(stm, pt, toSq);

    // If the moved piece was a pawn, also update the pawn hash key
    if (pt == PAWN) {
        hash_pawn(color, fromSq);
        hash_pawn(color, toSq);
    }

}

// Play a move on the board
void Board::do_move(const Move move) {

    assert(move != MOVE_NONE);

    assert(is_valid(move));
    assert(is_legal(move));

    const Square fromSq       = from_sq(move);
    const Square toSq         = to_sq(move);
    const MoveType moveType   = move_type(move);
    const Piecetype pieceType = pieceTypes[fromSq];
    const Piecetype captured  = pieceTypes[toSq];

    // Add the current game state to the list of previous states
    states.push_back(state);
    moves.push_back(move);

    // Update position hash key for enPassant and castlingRights
    hash_enPassant();
    hash_castling();

    state.captured = captured;
    state.checkers = 0;
    state.enPassant = SQUARE_NONE;
    state.fiftyMoves++;

    // NOTE: no need to check if piece on square -> pieces[PIECE_NONE] is trash

    // If there is a piece on the target square, remove it and reset the fifty moves counter
    if (captured != PIECE_NONE) {
        remove_piece(toSq);
        state.fiftyMoves = 0;
    }

    // Move the piece to its destination square
    move_piece(fromSq, toSq);

    switch(moveType) {

        case NORMAL:
        {
            if (pieceType == PAWN) {
                // Reset the fifty moves counter if we move with a pawn
                state.fiftyMoves = 0;
                if (std::abs(fromSq - toSq) == 2 * UP) {
                    if (PawnAttacks[stm][fromSq + direction(stm, UP)] & pieces(!stm, PAWN)) {
                        // Update the en-passant square
                        state.enPassant = fromSq + direction(stm, UP);
                        hash_enPassant();
                    }
                }
            }

            // Update castling state
            state.castlingRights &= CASTLE_MASK[fromSq];
        }
        break;

        case CASTLING:
            {
                const Square rookToSq   = toSq + ((toSq == SQUARE_G1 || toSq == SQUARE_G8) ?  1 : -1);
                const Square rookFromSq = toSq + ((toSq == SQUARE_G1 || toSq == SQUARE_G8) ? -1 :  2);

                // If we castle, we need to move the rook as well
                move_piece(rookFromSq, rookToSq);

                // Update castling state
                state.castlingRights &= CASTLE_MASK[fromSq];
            }
            break;

        case ENPASSANT:
            {
                const Square capSq = toSq + direction(stm, DOWN);

                // NOTE: do not assign pawn to state.captured!!!

                // Remove the pawn which has been captured en-passant
                remove_piece(capSq);
                state.fiftyMoves = 0;
            }
            break;

        default:
            {
                const Piecetype promotionType = prom_piecetype(moveType);

                // For promotions, we need to exchange the pawn for the piece specified in the move object
                remove_piece(toSq);
                add_piece(stm, promotionType, toSq);

                // Reset the fifty moves counter since we moved with a pawn
                state.fiftyMoves = 0;
            }
            break;

    }

    // TODO: Is this correct?
    // Update position hash key for castling rights
    hash_castling();

    // Switch turn and update position hash key for turn
    hash_turn();
    stm = !stm;
    hash_turn();

    // Update the combined colors bitboard
    bbColors[BOTH] = bbColors[WHITE] | bbColors[BLACK];

    // Update the king blockers and checkers
    update_check_info();

    // Update the repetition count
    update_repetition_count();

    // Increase the number of played half moves
    ply++;

}

// Undo the last move played on the board.
void Board::undo_move() {

    assert(!moves.empty());

    const Move move = moves.back();

    // Remove the last move from the move list
    moves.pop_back();

    const Square fromSq   = from_sq(move);
    const Square toSq     = to_sq(move);
    const MoveType moveType = move_type(move);

    // Move the piece back to its original square
    move_piece(toSq, fromSq);

    if (state.captured != PIECE_NONE) {
        add_piece(stm, state.captured, toSq);
    }

    switch(moveType) {

        case NORMAL: break;
        case CASTLING:
            {
                // For castling, we need to move the rook back to its initial position
                const Square rookToSq   = toSq + ((toSq == SQUARE_G1 || toSq == SQUARE_G8) ?  1 : -1);
                const Square rookFromSq = toSq + ((toSq == SQUARE_G1 || toSq == SQUARE_G8) ? -1 :  2);
                move_piece(rookToSq, rookFromSq);
            }
            break;

        case ENPASSANT:
            {
                // For en-passants, we need to put back the captured pawn
                const Square capSq = toSq + direction(!stm, DOWN);
                add_piece(stm, PAWN, capSq);
            }
            break;

        default:
            {
                // For promotions, we need to exchange the piece for the pawn again
                remove_piece(fromSq);
                add_piece(!stm, PAWN, fromSq);
            }
            break;

    }

    // Update the combined colors bitboard
    bbColors[BOTH] = bbColors[WHITE] | bbColors[BLACK];

    // Flip the turn
    stm = !stm;

    // Decrease the number of half-moves played
    ply--;

    // Revert to the previous board state
    state = states.back();
    states.pop_back();

}

// Do a null move on the board
void Board::do_nullmove() {

    states.push_back(state);

    hash_enPassant(); // hash_EnPassant() checks for SQUARE_NONE
    state.enPassant = SQUARE_NONE;

    hash_turn();
    stm = !stm;
    hash_turn();

    update_check_info();

    ply++;

}

// Undo a null move on the board
void Board::undo_nullmove() {

    stm = !stm;

    ply--;

    state = states.back();

    states.pop_back();

}

// Checks for draw by threefold repetition or 50 move rule.
// Stalemate is settled by search, insufficient material by evaluation
bool Board::check_draw() {

    return state.repetitionCount >= 2 || state.fiftyMoves >= 100 || is_material_draw();

}

// Checks wether a given move on the current board is pseudo-legal
// A move is invalid if it violates the rules of chess, however, this
// method does not check if the move is actually legal, meaning it does
// not check wether the own king might be left in check.
bool Board::is_valid(const Move move) const {

    const unsigned fromSq = from_sq(move);
    const unsigned toSq   = to_sq(move);

    // Move is illegal when there is no piece on the origin square, it's not the owners turn or it would capture a friendly piece
    if (   pieceTypes[fromSq] == PIECE_NONE
        || owner(fromSq) != stm
        || (SQUARES[toSq] & bbColors[stm]))
    {
        return false;
    }

    const MoveType moveType = move_type(move);

    if (moveType == NORMAL) {

        // Special rules for pawns
        if (pieceTypes[fromSq] == PAWN) {

            // Push to 8th rank invalid -> would be promotion, so move type can't be NORMAL
            if (relative_rank(stm, toSq) == 7) {
                return false;
            }

            if (   !((SQUARES[toSq] & PawnAttacks[stm][fromSq]) & bbColors[!stm]) // If the pawn moves diagonally but there is no enemy piece, the move is not valid
                && !((fromSq + direction(stm, UP) == toSq) && pieceTypes[toSq] == PIECE_NONE) // If the pawn pushes forward, there can be no piece in front of it
                && !(    relative_rank(stm, fromSq) == 1  // For double pushes, there cannot be any piece one and two squares ahead
                      && (fromSq + 2 * direction(stm, UP) == toSq)
                      && (pieceTypes[fromSq +     direction(stm, UP)] == PIECE_NONE)
                      && (pieceTypes[fromSq + 2 * direction(stm, UP)] == PIECE_NONE)))
                {
                    return false;
                }
        // If the piece cannot move to the target square, the move is not valid
        } else if (!(SQUARES[toSq] & piece_attacks(pieceTypes[fromSq], stm, fromSq))) {
            return false;
        }

    } else {
        if (moveType == CASTLING) {
            return is_castling_valid((int)toSq - (int)fromSq < 0 ? CASTLE_SHORT : CASTLE_LONG);
        }

        if (is_promotion(move)) {
            if (   !(pieceTypes[fromSq] == PAWN)
                || !(SQUARES[toSq] & PAWN_FINALRANK[stm])
                || !(SQUARES[toSq] & generate_pawn_moves(stm, fromSq, bbColors[BOTH], bbColors[!stm])))
            {
                return false;
            }
        }

        if (moveType == ENPASSANT) {
            return toSq == state.enPassant && (SQUARES[fromSq] & bbPieces[PAWN]);
        }
    }

    return true;

}
