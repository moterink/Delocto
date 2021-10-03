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

#include "movegen.hpp"
#include "board.hpp"
#include "bitboards.hpp"

// Appends the moves of a given move list to its own moves
void MoveList::merge(MoveList list) {

    unsigned int lcount;
    for (lcount = 0; lcount < (list.size-list.index); lcount++) {
        moves[size + lcount] = list.moves[list.index + lcount];
    }
    size += lcount;

}

// Swaps the position of two moves given their indices
void MoveList::swap(const unsigned index1, const unsigned index2) {

    std::iter_swap(moves.begin() + index1, moves.begin() + index2);
    std::iter_swap(scores.begin() + index1, scores.begin() + index2);

}

// Picks the move in the list with the highest score and returns its index
Move MoveList::pick() {

    if (size == 0) {
        return MOVE_NONE;
    }

    int bestScore = scores[index];
    unsigned bestIndex = index;

    for (unsigned mIndex = index; mIndex < size; mIndex++) {
        if (scores[mIndex] > bestScore) {
            bestScore = scores[mIndex];
            bestIndex = mIndex;
        }
    }

    swap(index, bestIndex);
    return moves[index];

}

// Prints all moves in the list
void MoveList::print() {

    for (unsigned mIndex = 0; mIndex < size; mIndex++) {
        print_move(moves[mIndex]);
    }

}

// Generates all pseudo-legal quiet promotions in a given position and adds them to the given move list
static void gen_quietproms(const Board& board, MoveList& moveList, const Color color, const Bitboard targets) {

    // Bitboard of all pawns for the given color which are one square away from promotion
    Bitboard pawns = board.pieces(color, PAWN) & PAWN_STARTRANK[!color];

    while (pawns) {

        const Square fromSq = pop_lsb(pawns);
        const Square toSq   = fromSq + direction(color, UP);

        if (SQUARES[toSq] & targets) {
            moveList.append(make_move(fromSq, toSq, PROMOTION_QUEEN));
            moveList.append(make_move(fromSq, toSq, PROMOTION_ROOK));
            moveList.append(make_move(fromSq, toSq, PROMOTION_BISHOP));
            moveList.append(make_move(fromSq, toSq, PROMOTION_KNIGHT));
        }

    }

}

// Generates all pseudo-legal promotions in a given position which capture a piece and adds them to the given move list
static void gen_capproms(const Board& board, MoveList& moveList, const Color color, const Bitboard targets) {

    Bitboard pawns = board.pieces(color, PAWN) & PAWN_STARTRANK[!color];

    while (pawns) {

        const Square fromSq = pop_lsb(pawns);
        Bitboard caps = PawnAttacks[color][fromSq] & targets;

        while (caps) {

            const Square toSq = pop_lsb(caps);

            moveList.append(make_move(fromSq, toSq, PROMOTION_QUEEN));
            moveList.append(make_move(fromSq, toSq, PROMOTION_ROOK));
            moveList.append(make_move(fromSq, toSq, PROMOTION_BISHOP));
            moveList.append(make_move(fromSq, toSq, PROMOTION_KNIGHT));

        }

    }

}

// Generates the pseudo-legal en-passant captures (if possible) for a given position and adds them to the given move list
static void gen_ep(const Board& board, MoveList& moveList, const Color color, const Bitboard targets) {

    const Square epSq = board.enpassant_square();

    if (epSq != SQUARE_NONE && (SQUARES[epSq] & targets)) {

        // Bitboard of all friendly pawns attacking the en-passant square
        Bitboard pawns = PawnAttacks[!color][epSq] & board.pieces(color, PAWN);

        while (pawns) {

            moveList.append(make_move(pop_lsb(pawns), epSq, ENPASSANT));

        }

    }

}

// Generates all pseudo-legal moves capturing pieces for a given color and adds them to the move list
// For each piece, the method creates a bitboard with all possible target squares. It then loops over each
// set bit on the bitboard and adds a move for each to the move list until there are not bits left on the bitboard.
static void gen_piece_caps(const Board& board, MoveList& moveList, const Color color, Bitboard targets) {

    const Square sq = lsb_index(board.pieces(color, KING));
    Bitboard moves = KingAttacks[sq] & board.pieces(!color);

    while (moves) {

        moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

    }

    Bitboard pawns = board.pieces(color, PAWN) & ~PAWN_STARTRANK[!color];
    while (pawns) {

        const Square sq = pop_lsb(pawns);
        Bitboard moves = PawnAttacks[color][sq] & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    Bitboard knights = board.pieces(color, KNIGHT);
    while (knights) {

        const Square sq = pop_lsb(knights);
        Bitboard moves = KnightAttacks[sq] & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    Bitboard bishops = board.pieces(color, BISHOP);
    while (bishops) {

        const Square sq = pop_lsb(bishops);
        Bitboard moves = gen_bishop_moves(sq, board.pieces(BOTH), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    Bitboard rooks = board.pieces(color, ROOK);
    while (rooks) {

        const Square sq = pop_lsb(rooks);
        Bitboard moves = gen_rook_moves(sq, board.pieces(BOTH), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }
    }

    Bitboard queens = board.pieces(color, QUEEN);
    while (queens) {

        const Square sq = pop_lsb(queens);
        Bitboard moves = get_queen_moves(sq, board.pieces(BOTH), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

}

// Same as the function above, only for quiet moves though
static void gen_piece_quiets(const Board& board, MoveList& moveList, const Color color, const Bitboard targets) {

    const Bitboard pawns      = board.pieces(color, PAWN) & ~PAWN_STARTRANK[!color];
    const Bitboard pawnPushes = shift_up(pawns, color) & ~board.pieces(BOTH);
    Bitboard singlePushes     = pawnPushes & targets;
    Bitboard doublePushes     = shift_up(pawnPushes & PAWN_FIRST_PUSH_RANK[color], color) & targets;

    Bitboard knights = board.pieces(color, KNIGHT);
    while (knights) {

        const Square sq = pop_lsb(knights);
        Bitboard moves = KnightAttacks[sq] & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    Bitboard bishops = board.pieces(color, BISHOP);
    while (bishops) {

        const Square sq = pop_lsb(bishops);
        Bitboard moves = gen_bishop_moves(sq, board.pieces(BOTH), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    Bitboard rooks = board.pieces(color, ROOK);
    while (rooks) {

        const Square sq = pop_lsb(rooks);
        Bitboard moves = gen_rook_moves(sq, board.pieces(BOTH), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }
    }

    Bitboard queens = board.pieces(color, QUEEN);
    while (queens) {

        const Square sq = pop_lsb(queens);
        Bitboard moves = get_queen_moves(sq, board.pieces(BOTH), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    while (singlePushes) {

        const Square toSq = pop_lsb(singlePushes);
        const Square fromSq = lsb_index(shift_down(SQUARES[toSq], color));
        moveList.append(make_move(fromSq, toSq, NORMAL));

    }

    while (doublePushes) {

        const Square toSq = pop_lsb(doublePushes);
        const Square fromSq = lsb_index(shift_down(shift_down(SQUARES[toSq], color), color));
        moveList.append(make_move(fromSq, toSq, NORMAL));

    }

    const Square sq = lsb_index(board.pieces(color, KING));
    Bitboard moves = gen_king_moves(sq, board.pieces(color)) & ~board.pieces(BOTH);

    while (moves) {

        moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

    }

}

// Returns true if the given castling is possible in the current position, false if not.
bool Board::is_castling_valid(const unsigned type) const {

    if (!checkers()) {

        return (   state.castlingRights & CASTLE_TYPES[stm][type]
                && pieces(stm, KING)    & SQUARES[KING_INITIAL_SQUARE[stm]]
                && pieces(stm, ROOK)    & SQUARES[CASTLE_ROOK_ORIGIN_SQUARE[stm][type]]
                && !(pieces(BOTH)       & CASTLE_PATH[stm][type]));

    }

    return false;

}

// Generates all castling moves for the given position or a given color and adds them to the move list
static void gen_castlings(const Board& board, MoveList& moveList, const Color color) {

    if (board.is_castling_valid(CASTLE_SHORT)) {
        moveList.append(CASTLE_MOVES[color][CASTLE_SHORT]);
    }

    if (board.is_castling_valid(CASTLE_LONG)) {
        moveList.append(CASTLE_MOVES[color][CASTLE_LONG]);
    }

}

// Generates all possible moves evading a check for the given color in the given position and adds all evasions to the move list
MoveList gen_evasions(const Board& board, const MoveGenType mtype) {

    assert(board.checkers());

    MoveList moveList;

    const Color color       = board.turn();
    const Square ksq        = lsb_index(board.pieces(color, KING));
    const Bitboard checkers = board.checkers();
    const Bitboard sliders  = checkers & ~(board.pieces(color, KNIGHT) | board.pieces(color, PAWN));

    if (popcount(checkers) >= 2) {

        Bitboard kingMoves = KingAttacks[ksq] & ((mtype == MOVES_QUIETS) ? ~board.pieces(BOTH)
                                                                         : (mtype == MOVES_CAPTURES) ? board.pieces(!color)
                                                                         : ~board.pieces(color));
        while (kingMoves) {
            moveList.append(make_move(ksq, pop_lsb(kingMoves), NORMAL));
        }
        return moveList;

    }

    switch(mtype) {

        case MOVES_QUIETS:
        {
            const Bitboard targets = (sliders ? (RayTable[lsb_index(sliders)][ksq] & ~board.pieces(BOTH)) : ~board.pieces(BOTH));
            gen_quietproms(board, moveList, color, targets);
            gen_piece_quiets(board, moveList, color, targets);
            break;
        }
        case MOVES_CAPTURES:
        {
            gen_capproms(board, moveList, color, checkers);
            gen_piece_caps(board, moveList, color, checkers);
            if (checkers & board.pieces(!color, PAWN)) {
                gen_ep(board, moveList, color, SQUARES[board.enpassant_square()]);
            }
            break;
        }
        case MOVES_ALL:
        {
            const Bitboard targets = (sliders ? (RayTable[lsb_index(sliders)][ksq] & ~board.pieces(BOTH)) : ~board.pieces(BOTH));
            gen_quietproms(board, moveList, color, targets);
            gen_piece_quiets(board, moveList, color, targets);
            gen_capproms(board, moveList, color, checkers);
            gen_piece_caps(board, moveList, color, checkers);
            if (checkers & board.pieces(!color, PAWN)) {
                gen_ep(board, moveList, color, SQUARES[board.enpassant_square()]);
            }
            break;
        }
        default:
            assert(false);

    }

    return moveList;

}

// Given a move list and a board, this function returns a new move list
// containing only the legal moves of the original list
MoveList gen_legals(const Board& board, const MoveList& moves) {

    MoveList legals;

    for (unsigned m = 0; m < moves.size; m++) {

        const Move move = moves.moves[m];

        if (board.is_legal(move)) {
            legals.append(move);
        }

    }

    return legals;

}

// Generates all possible pseudo-legal quiet moves in the given position for a given color
MoveList gen_quiets(const Board& board, const Color color) {

    MoveList moveList;

    if (board.checkers()) {
        return gen_evasions(board, MOVES_QUIETS);
    } else {
        const Bitboard targets = ~board.pieces(BOTH);
        gen_quietproms(board, moveList, color, targets);
        gen_castlings(board, moveList, color);
        gen_piece_quiets(board, moveList, color, targets);
    }

    return moveList;

}

// Generates all possible pseudo-legal moves capturing a piece in the given position for a given color
MoveList gen_caps(const Board& board, const Color color) {

    MoveList moveList;

    if (board.checkers()) {
        return gen_evasions(board, MOVES_CAPTURES);
    } else {
        const Bitboard targets = board.pieces(!color);
        gen_capproms(board, moveList, color, targets);
        gen_piece_caps(board, moveList, color, targets);
        gen_ep(board, moveList, color, SQUARES[board.enpassant_square()]);
    }

    return moveList;

}

// Generates all possible pseudo-legal moves for a given position for a given color
MoveList gen_all(const Board& board, const Color color) {

    MoveList moveList;

    moveList.merge(gen_quiets(board, color));
    moveList.merge(gen_caps(board, color));

    return moveList;

}

// Checks wether a move is giving check to the opponent's king
bool Board::gives_check(const Move move) {

    const Square ksq    = lsb_index(pieces(!stm, KING));
    const Square fromSq = from_sq(move);
    const Square toSq   = to_sq(move);

    Bitboard attacks = 0;

    switch(pieceTypes[fromSq]) {

        case PAWN:
            attacks = PawnAttacks[stm][toSq];
            break;

        case KNIGHT:
            attacks = KnightAttacks[toSq];
            break;

        case BISHOP:
            attacks = gen_bishop_moves(toSq, bbColors[BOTH], bbColors[stm]);
            break;

        case ROOK:
            attacks = gen_rook_moves(toSq, bbColors[BOTH], bbColors[stm]);
            break;

        case QUEEN:
            attacks = get_queen_moves(toSq, bbColors[BOTH], bbColors[stm]);
            break;

        case KING:
            break;

        default:
            assert(false);

    }

    // Check if the piece attacks the opponents king directly
    if (attacks & SQUARES[ksq]) {
        return true;
    }

    // Check for discovered checks from sliders which appear by moving the piece
    if (slider_attackers_discovered(stm, ksq, fromSq, toSq)) {
        return true;
    }

    return false;

}
