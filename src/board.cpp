/*
  Delocto Chess Engine
  Copyright (c) 2018-2019 Moritz Terink

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
#include <ctype.h>

static const unsigned int CastleMask[64] = {

    14, 15, 15, 12, 15, 15, 15, 13,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    11, 15, 15,  3, 15, 15, 15,  7

};

// Reset board
void Board::clear() {

    states.clear();
    moves.clear();

    bbColors[WHITE] = 0;
    bbColors[BLACK] = 0;
    bbColors[BOTH] = 0;

    for (Color c = WHITE; c < BOTH; c++) {
        for (Piecetype pt = PAWN; pt < PIECE_NONE; pt++) {
            bbPieces[pt] = 0;
            pieceCounts[c][pt] = 0;
        }
    }
    std::fill(pieceTypes, pieceTypes+64, PIECE_NONE);

    state.enPassant = SQUARE_NONE;
    state.castling = 0;
    state.fiftyMoves = 0;
    state.material[WHITE] = V(0, 0);
    state.material[BLACK] = V(0, 0);
    state.captured = PIECE_NONE;
    state.pst[WHITE] = V(0, 0);
    state.pst[BLACK] = V(0, 0);
    state.kingBlockers[WHITE] = 0;
    state.kingBlockers[BLACK] = 0;
    state.checkers = 0;

    ply = 0;

    stm = WHITE;

}

// Calculate hash, pawn and material key
void Board::calc_keys() {

    state.hashKey = 0;
    state.pawnKey = 0;
    state.materialKey = 0;

    uint64_t temp = bbColors[BOTH];

    while (temp) {

        const unsigned sq  = pop_lsb(temp);
        const Color color  = owner(sq);
        const Piecetype pt = pieceTypes[sq];

        hash_piece(color, pt, sq);

        if (pt == PAWN) {
            hash_pawn(color, sq);
        }

    }

    for (Color c = WHITE; c < BOTH; c++) {
        for (Piecetype pt = PAWN; pt < PIECE_NONE; pt++) {
            hash_material(c, pt);
        }
    }

    hash_enPassant();
    hash_castling();
    hash_turn();

}

void Board::update_check_info() {

    state.kingBlockers[WHITE] = get_slider_blockers(bbColors[BLACK], lsb_index(pieces(WHITE, KING)));
    state.kingBlockers[BLACK] = get_slider_blockers(bbColors[WHITE], lsb_index(pieces(BLACK, KING)));

    state.checkers = sq_attackers(!stm, lsb_index(pieces(stm, KING)), bbColors[BOTH]);

}

uint64_t Board::get_slider_blockers(const uint64_t sliders, const unsigned sq) const {

    uint64_t blockers = 0;
    uint64_t pinners  = ((BishopAttacks[sq] & (bbPieces[BISHOP] | bbPieces[QUEEN])) | (RookAttacks[sq] & (bbPieces[ROOK] | bbPieces[QUEEN]))) & sliders;
    uint64_t occupied = bbColors[BOTH] ^ pinners ^ SQUARES[sq];

    while (pinners) {

        const unsigned psq = pop_lsb(pinners);
        const uint64_t pin = RayTable[psq][sq] & occupied;

        if (popcount(pin) == 1) {
            blockers |= pin;
        }

    }

    return blockers;

}

int Board::mvvlva(const Move move) const {

    if (is_ep(move)) {
        return MvvLvaVictim[PAWN] - MvvLvaAttacker[PAWN];
    }

    int value = 0;

    if (is_promotion(move)) {
        value += MvvLvaVictim[prom_piecetype(move_type(move))];
    }

    value += MvvLvaVictim[pieceTypes[to_sq(move)]] - MvvLvaAttacker[pieceTypes[from_sq(move)]];

    return value;

}

// Get board from fen string
void Board::set_fen(std::string fen) {

    clear();

    std::string nums = "12345678";

    unsigned i;
    unsigned r = 0;
    unsigned f = 0;

    for (i = 0; fen[i] != ' '; i++) {

        unsigned sq = 63 - square(f, r);

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
                default: assert(false);

            }

            state.pst[color]      += PieceSquareTable[color][type][sq];
            state.material[color] += Material[type];
            bbColors[color] |= SQUARES[sq];
            bbColors[BOTH]  |= SQUARES[sq];
            bbPieces[type]  |= SQUARES[sq];
            pieceTypes[sq] = type;
            pieceCounts[color][type]++;
            f++;
        }

    }

    ++i;
    stm = (fen[i] == 'w') ? WHITE : BLACK;
    i += 2;

    while (fen[i] != ' ') {

        switch (fen[i]) {

            case 'K': { state.castling |= WKCASFLAG; break; }
            case 'Q': { state.castling |= WQCASFLAG; break; }
            case 'k': { state.castling |= BKCASFLAG; break; }
            case 'q': { state.castling |= BQCASFLAG; break; }

        }

        ++i;

    }

    ++i;

    state.enPassant = (fen[i] != '-') ? square(7 - (fen[i] - 'a'), fen[i + 1] - '1') : SQUARE_NONE;

    i += 2;

    if (state.enPassant != SQUARE_NONE) {
        i++;
    }

    if (fen.size() >= i && isdigit(fen[i])) {
        std::string cut = fen.substr(i);
        state.fiftyMoves = std::stoi(cut.substr(0, cut.find(" ")));
    } else {
        state.fiftyMoves = 0;
    }

    update_check_info();
    calc_keys();

}

// Print board to screen for debuging purposals
void Board::print() const {

    char labels[2][6] = { { 'P', 'N', 'B', 'R', 'Q', 'K' }, { 'p', 'n', 'b', 'r', 'q', 'k' } };
    unsigned r  = 0;
    unsigned sq = 0;

    while (sq != SQUARE_NONE) {

        if (rank(sq) != r) {
            std::cout << std::endl;
            r++;
        }

        if (pieceTypes[sq] != PIECE_NONE) {
            std::cout << labels[owner(sq)][pieceTypes[sq]] << ' ';
        } else {
            std::cout << ". ";
        }
        sq++;
    }

    std::cout << std::endl << std::endl;

}

void Board::add_piece(const Color color, const Piecetype pt, const unsigned sq) {

    hash_material(color, pt);

    bbColors[color] |= SQUARES[sq];
    bbPieces[pt]    |= SQUARES[sq];
    pieceTypes[sq]  = pt;
    pieceCounts[color][pt]++;

    state.material[color] += Material[pt];
    state.pst[color]      += PieceSquareTable[color][pt][sq];

    hash_material(color, pt);
    hash_piece(color, pt, sq);
    if (pt == PAWN) {
        hash_pawn(color, sq);
    }

}

void Board::remove_piece(const unsigned sq) {

    const Color color  = owner(sq);
    const Piecetype pt = pieceTypes[sq];

    hash_material(color, pt);

    bbColors[color] ^= SQUARES[sq];
    bbPieces[pt]    ^= SQUARES[sq];
    pieceTypes[sq]  = PIECE_NONE;
    pieceCounts[color][pt]--;

    state.material[color] -= Material[pt];
    state.pst[color]      -= PieceSquareTable[color][pt][sq];

    hash_material(color, pt);
    hash_piece(color, pt, sq);
    if (pt == PAWN) {
        hash_pawn(color, sq);
    }

}

void Board::move_piece(const unsigned fromSq, const unsigned toSq) {

    const Color color  = owner(fromSq);
    const Piecetype pt = pieceTypes[fromSq];

    bbColors[color] ^= SQUARES[fromSq];
    bbPieces[pt]    ^= SQUARES[fromSq];
    bbColors[color] |= SQUARES[toSq];
    bbPieces[pt]    |= SQUARES[toSq];

    pieceTypes[fromSq] = PIECE_NONE;
    pieceTypes[toSq] = pt;

    state.pst[color] -= PieceSquareTable[color][pt][fromSq];
    state.pst[color] += PieceSquareTable[color][pt][toSq];

    hash_piece(stm, pt, fromSq);
    hash_piece(stm, pt, toSq);

    if (pt == PAWN) {
        hash_pawn(color, fromSq);
        hash_pawn(color, toSq);
    }

}

bool Board::do_move(const Move move) {

    assert(move != MOVE_NONE);

    const int fromSq        = from_sq(move);
    const int toSq          = to_sq(move);
    const MoveType moveType = move_type(move);
    const Piecetype fromPt  = pieceTypes[fromSq];
    const Piecetype toPt    = pieceTypes[toSq];

    states.push_back(state);
    moves.push_back(move);

    hash_enPassant();
    hash_castling();

    state.captured = toPt;
    state.checkers = 0;
    state.enPassant = SQUARE_NONE;
    state.fiftyMoves++;

    // NOTE: no need to check if piece on square -> pieces[PIECE_NONE] is trash

    if (toPt != PIECE_NONE) {
        remove_piece(toSq);
        state.fiftyMoves = 0;
    }

    move_piece(fromSq, toSq);

    switch(moveType) {

        case NORMAL:
            {
                // Update enpassant square
                if (fromPt == PAWN) {
                    state.fiftyMoves = 0;
                    if (std::abs(fromSq - toSq) == 16) {
                        if (PawnAttacks[stm][fromSq + DIRECTIONS[stm][UP]] & pieces(!stm, PAWN)) {
                            state.enPassant = fromSq + DIRECTIONS[stm][UP];
                            hash_enPassant();
                        }
                    }
                }

                state.castling &= CastleMask[fromSq];
            }
            break;

        case CASTLING:
            {
                const unsigned rookToSq   = toSq + ((toSq == G1 || toSq == G8) ?  1 : -1);
                const unsigned rookFromSq = toSq + ((toSq == G1 || toSq == G8) ? -1 :  2);

                move_piece(rookFromSq, rookToSq);

                state.castling &= CastleMask[fromSq];
            }
            break;

        case ENPASSANT:
            {
                const unsigned capSq = toSq + DIRECTIONS[stm][DOWN];

                // NOTE: do not assign pawn to state.captured!!!

                remove_piece(capSq);
                state.fiftyMoves = 0;
            }
            break;

        default:

            {
                const Piecetype promotionType = prom_piecetype(moveType);

                remove_piece(toSq);
                add_piece(stm, promotionType, toSq);

                state.fiftyMoves = 0;
            }
            break;

    }

    hash_castling();

    hash_turn();
    stm = !stm;
    hash_turn();

    bbColors[BOTH] = bbColors[WHITE] | bbColors[BLACK];

    update_check_info();

    ply++;

    return true;

}

void Board::undo_move() {

    assert(!moves.empty());

    const Move move = moves.back();

    moves.pop_back();

    const unsigned fromSq   = from_sq(move);
    const unsigned toSq     = to_sq(move);
    const MoveType moveType = move_type(move);

    move_piece(toSq, fromSq);

    if (state.captured != PIECE_NONE) {
        add_piece(stm, state.captured, toSq);
    }

    switch(moveType) {

        case NORMAL: break;
        case CASTLING:
            {
                const unsigned rookToSq   = toSq + ((toSq == G1 || toSq == G8) ?  1 : -1);
                const unsigned rookFromSq = toSq + ((toSq == G1 || toSq == G8) ? -1 :  2);
                move_piece(rookToSq, rookFromSq);
            }
            break;

        case ENPASSANT:
            {
                const unsigned capSq = toSq + DIRECTIONS[!stm][DOWN];
                add_piece(stm, PAWN, capSq);
            }
            break;

        default:
            {
                remove_piece(fromSq);
                add_piece(!stm, PAWN, fromSq);
            }
            break;

    }

    bbColors[BOTH] = bbColors[WHITE] | bbColors[BLACK];

    stm = !stm;

    ply--;

    state = states.back();
    states.pop_back();

}

// Make a null move on the board
void Board::do_nullmove() {

    states.push_back(state);

    hash_enPassant(); // hashEnPassant checks for SQUARE_NONE
    state.enPassant = SQUARE_NONE;

    hash_turn();
    stm = !stm;
    hash_turn();

    update_check_info();

    ply++;

}

// Unmake a null move on the board
void Board::undo_nullmove() {

    stm = !stm;

    ply--;

    state = states.back();

    states.pop_back();

}

// Checks for threefold repetition and 50 move rule. Stalemate is settled by search
bool Board::checkDraw() {

    if (state.fiftyMoves >= 100) {
        return true;
    }

    if (is_material_draw()) {
        return true;
    }

    unsigned count = 0;
    for (unsigned i = 0; i < states.size(); i++) {
        if (states[i].hashKey == state.hashKey) {
            count++;
        }
    }

    if (count >= 2) {
        return true;
    }

    return false;

}

bool Board::is_valid(const Move move) const {

    const unsigned fromSq = from_sq(move);
    const unsigned toSq   = to_sq(move);

    if (pieceTypes[fromSq] == PIECE_NONE || owner(fromSq) != stm || (SQUARES[toSq] & bbColors[stm])) {
        return false;
    }

    const MoveType moveType = move_type(move);

    if (moveType == NORMAL) {
        if (SQUARES[fromSq] & bbPieces[PAWN]) {
            // Push to 8th rank invalid -> would be promotion, so mtype can't be NORMAL
            if (relative_rank(stm, toSq) == 7) {
                return false;
            }
            if (!((SQUARES[toSq] & PawnAttacks[stm][fromSq]) & bbColors[!stm])
               && !((fromSq + DIRECTIONS[stm][UP] == toSq) && pieceTypes[toSq] == PIECE_NONE)
               && !(relative_rank(stm, fromSq) == 1 && (fromSq + 2 * DIRECTIONS[stm][UP] == toSq) && (pieceTypes[fromSq + DIRECTIONS[stm][UP]] == PIECE_NONE) && (pieceTypes[fromSq + 2 * DIRECTIONS[stm][UP]] == PIECE_NONE))) {
                    return false;
            }
        } else if (!(SQUARES[toSq] & pseudo_bb(pieceTypes[fromSq], stm, fromSq))) {
            return false;
        }
    } else {
        if (moveType == CASTLING) {
            if (is_castling_valid(castleByKingpos[toSq]) == false) {
                return false;
            }
        }

        if (is_promotion(move)) {
            if (!(SQUARES[fromSq] & bbPieces[PAWN]) || !(SQUARES[toSq] & PAWN_FINALRANK[stm]) || !(SQUARES[toSq] & generate_pawn_moves(stm, fromSq, bbColors[BOTH], bbColors[!stm]))) {
                return false;
            }
        }

        if (moveType == ENPASSANT) {
            if (toSq != state.enPassant || !(SQUARES[fromSq] & bbPieces[PAWN])) {
                return false;
            }
        }
    }

    if (checkers()) {
        if (SQUARES[fromSq] != pieces(stm, KING)) {

            const unsigned ksq = lsb_index(pieces(stm, KING));

            if (popcount(checkers()) >= 2) {
                return false;
            }

            if (!(((RayTable[lsb_index(checkers())][ksq] | checkers())) & SQUARES[toSq])) {
                return false;
            }

        } else if (sq_attackers(!stm, toSq, (bbColors[BOTH] ^ SQUARES[fromSq])) & bbColors[!stm]) {
            return false;
        }
    }

    return true;

}
