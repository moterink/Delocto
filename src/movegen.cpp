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

#include "movegen.hpp"
#include "board.hpp"
#include "bitboards.hpp"

static const uint64_t WKCAS_BLOCKERS = SQUARES[F1] | SQUARES[G1];
static const uint64_t WQCAS_BLOCKERS = SQUARES[B1] | SQUARES[C1] | SQUARES[D1];
static const uint64_t BKCAS_BLOCKERS = SQUARES[F8] | SQUARES[G8];
static const uint64_t BQCAS_BLOCKERS = SQUARES[B8] | SQUARES[C8] | SQUARES[D8];

static const Move WKCAS_MOVE = make_move(E1, G1, CASTLING);
static const Move WQCAS_MOVE = make_move(E1, C1, CASTLING);
static const Move BKCAS_MOVE = make_move(E8, G8, CASTLING);
static const Move BQCAS_MOVE = make_move(E8, C8, CASTLING);

void MoveList::merge(MoveList list) {

    unsigned int lcount;
    for (lcount = 0; lcount < (list.size-list.index); lcount++) {
        moves[size + lcount] = list.moves[list.index + lcount];
    }
    size += lcount;

}

unsigned int MoveList::find(const Move move) {

    for (unsigned int mcount = 0; mcount < size; mcount++) {
        if (moves[mcount] == move) {
            return mcount;
        }
    }
    return size;

}

void MoveList::swap(const unsigned int index1, const unsigned int index2) {

    Move move1 = moves[index1];
    Move move2 = moves[index2];

    int value1 = values[index1];
    int value2 = values[index2];

    moves[index1] = move2;
    moves[index2] = move1;

    values[index1] = value2;
    values[index2] = value1;

}

Move MoveList::pick() {

    if (size == 0) return MOVE_NONE;

    int bestvalue = values[index];
    unsigned bestindex = index;

    for (unsigned mcount = index; mcount < size; mcount++) {
        if (values[mcount] > bestvalue) {
            bestvalue = values[mcount];
            bestindex = mcount;
        }
    }

    swap(index, bestindex);
    return moves[index];

}

void MoveList::print() {

    for (unsigned mcount = 0; mcount < size; mcount++) {
        print_move(moves[mcount]);
    }

}

static void gen_quietproms(const Board& board, MoveList& moveList, const Color color, const uint64_t targets) {

    uint64_t pawns = board.pieces(color, PAWN) & PAWN_STARTRANK[!color];

    while (pawns) {

        const unsigned int fromsq = pop_lsb(pawns);
        const unsigned int tosq = fromsq + DIRECTIONS[color][UP];

        if (SQUARES[tosq] & targets) {
            moveList.append(make_move(fromsq, tosq, QUEENPROM));
            moveList.append(make_move(fromsq, tosq, ROOKPROM));
            moveList.append(make_move(fromsq, tosq, BISHOPPROM));
            moveList.append(make_move(fromsq, tosq, KNIGHTPROM));
        }

    }

}

static void gen_capproms(const Board& board, MoveList& moveList, const Color color, const uint64_t targets) {

    uint64_t pawns = board.pieces(color, PAWN) & PAWN_STARTRANK[!color];

    while (pawns) {

        const unsigned fromsq = pop_lsb(pawns);
        uint64_t caps = PawnAttacks[color][fromsq] & targets;

        while (caps) {

            const unsigned int tosq = pop_lsb(caps);

            moveList.append(make_move(fromsq, tosq, QUEENPROM));
            moveList.append(make_move(fromsq, tosq, ROOKPROM));
            moveList.append(make_move(fromsq, tosq, BISHOPPROM));
            moveList.append(make_move(fromsq, tosq, KNIGHTPROM));

        }

    }

}

static void gen_ep(const Board& board, MoveList& moveList, const Color color, const uint64_t targets) {

    const unsigned epsq = board.enPassant();

    if (epsq != SQUARE_NONE && (SQUARES[epsq] & targets)) {

        uint64_t pawns = PawnAttacks[!color][epsq] & board.pieces(color, PAWN);

        while (pawns) {

            moveList.append(make_move(pop_lsb(pawns), epsq, ENPASSANT));

        }

    }

}

static void gen_piece_caps(const Board& board, MoveList& moveList, const Color color, uint64_t targets) {

    const unsigned sq = lsb_index(board.pieces(color, KING));
    uint64_t moves = KingAttacks[sq] & board.pieces(!color);

    while (moves) {

        moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

    }

    uint64_t pawns = board.pieces(color, PAWN) & ~PAWN_STARTRANK[!color];
    while (pawns) {

        const unsigned sq = pop_lsb(pawns);
        uint64_t moves = PawnAttacks[color][sq] & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    uint64_t knights = board.pieces(color, KNIGHT);
    while (knights) {

        const unsigned sq = pop_lsb(knights);
        uint64_t moves = KnightAttacks[sq] & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    uint64_t bishops = board.pieces(color, BISHOP);
    while (bishops) {

        const unsigned sq = pop_lsb(bishops);
        uint64_t moves = gen_bishop_moves(sq, board.pieces(BOTH), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    uint64_t rooks = board.pieces(color, ROOK);
    while (rooks) {

        const unsigned sq = pop_lsb(rooks);
        uint64_t moves = gen_rook_moves(sq, board.pieces(BOTH), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }
    }

    uint64_t queens = board.pieces(color, QUEEN);
    while (queens) {

        const unsigned sq = pop_lsb(queens);
        uint64_t moves = get_queen_moves(sq, board.pieces(BOTH), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

}

static void gen_piece_quiets(const Board& board, MoveList& moveList, const Color color, const uint64_t targets) {

    const uint64_t pawns      = board.pieces(color, PAWN) & ~PAWN_STARTRANK[!color];
    const uint64_t pawnPushes = shift_up(pawns, color) & ~board.pieces(BOTH);
    uint64_t singlePushes     = pawnPushes & targets;
    uint64_t doublePushes     = shift_up(pawnPushes & PAWN_FIRST_PUSH_RANK[color], color) & targets;

    uint64_t knights = board.pieces(color, KNIGHT);
    while (knights) {

        const unsigned sq = pop_lsb(knights);
        uint64_t moves = KnightAttacks[sq] & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    uint64_t bishops = board.pieces(color, BISHOP);
    while (bishops) {

        const unsigned sq = pop_lsb(bishops);
        uint64_t moves = gen_bishop_moves(sq, board.pieces(BOTH), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    uint64_t rooks = board.pieces(color, ROOK);
    while (rooks) {

        const unsigned sq = pop_lsb(rooks);
        uint64_t moves = gen_rook_moves(sq, board.pieces(BOTH), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }
    }

    uint64_t queens = board.pieces(color, QUEEN);
    while (queens) {

        const unsigned sq = pop_lsb(queens);
        uint64_t moves = get_queen_moves(sq, board.pieces(BOTH), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    while (singlePushes) {

        const unsigned tosq = pop_lsb(singlePushes);
        const unsigned fromsq = lsb_index(shift_down(SQUARES[tosq], color));
        moveList.append(make_move(fromsq, tosq, NORMAL));

    }

    while (doublePushes) {

        const unsigned tosq = pop_lsb(doublePushes);
        const unsigned fromsq = lsb_index(shift_down(shift_down(SQUARES[tosq], color), color));
        moveList.append(make_move(fromsq, tosq, NORMAL));

    }

    const unsigned sq = lsb_index(board.pieces(color, KING));
    uint64_t moves = gen_king_moves(sq, board.pieces(color)) & ~board.pieces(BOTH);

    while (moves) {

        moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

    }

}

bool Board::is_castling_valid(const unsigned flag) const {

    if (!checkers()) {

        switch (flag) {

            case WKCASFLAG:
            {
                return (   state.castling & WKCASFLAG
                        && pieces(WHITE, KING) & SQUARES[E1]
                        && pieces(WHITE, ROOK) & SQUARES[H1]
                        && !getAllOccupancy(WKCAS_BLOCKERS));

            }
            case WQCASFLAG:
            {
                return (   state.castling & WQCASFLAG
                        && pieces(WHITE, KING) & SQUARES[E1]
                        && pieces(WHITE, ROOK) & SQUARES[A1]
                        && !getAllOccupancy(WQCAS_BLOCKERS));


            }
            case BKCASFLAG:
            {
                return (   state.castling & BKCASFLAG
                        && pieces(BLACK, KING) & SQUARES[E8]
                        && pieces(BLACK, ROOK) & SQUARES[H8]
                        && !getAllOccupancy(BKCAS_BLOCKERS));

            }
            case BQCASFLAG:
            {
                return (   state.castling & BQCASFLAG
                        && pieces(BLACK, KING) & SQUARES[E8]
                        && pieces(BLACK, ROOK) & SQUARES[A8]
                        && !getAllOccupancy(BQCAS_BLOCKERS));
            }
        }
    }

    return false;

}

static void gen_castlings(const Board& board, MoveList& moveList, const Color color) {

    if (color == WHITE) {

        if (board.is_castling_valid(WKCASFLAG)) {
            moveList.append(WKCAS_MOVE);
        }

        if (board.is_castling_valid(WQCASFLAG)) {
            moveList.append(WQCAS_MOVE);
        }

    } else {

        if (board.is_castling_valid(BKCASFLAG)) {
            moveList.append(BKCAS_MOVE);
        }

        if (board.is_castling_valid(BQCASFLAG)) {
            moveList.append(BQCAS_MOVE);
        }

    }

}

void gen_evasions(const Board& board, const MoveGenType mtype, MoveList& moveList, const Color color) {

    const unsigned ksq      = lsb_index(board.pieces(color, KING));
    const uint64_t checkers = board.checkers();
    const uint64_t sliders  = checkers & ~(board.pieces(color, KNIGHT) | board.pieces(color, PAWN));

    if (popcount(checkers) >= 2) {

        uint64_t kingMoves = KingAttacks[ksq] & ((mtype == MOVES_QUIETS) ? ~board.pieces(BOTH) : board.pieces(!color));
        while (kingMoves) {
            moveList.append(make_move(ksq, pop_lsb(kingMoves), NORMAL));
        }
        return;

    }

    switch(mtype) {

        case MOVES_QUIETS:
        {
            const uint64_t targets = (sliders ? (RayTable[lsb_index(sliders)][ksq] & ~board.pieces(BOTH)) : ~board.pieces(BOTH));
            gen_quietproms(board, moveList, color, targets);
            gen_piece_quiets(board, moveList, color, targets);
            break;
        }
        case MOVES_CAPTURES:
        {
            gen_capproms(board, moveList, color, checkers);
            gen_piece_caps(board, moveList, color, checkers);
            if (checkers & board.pieces(!color, PAWN)) {
                gen_ep(board, moveList, color, SQUARES[board.enPassant()]);
            }
            break;
        }
        case MOVES_ALL:
        {
            const uint64_t targets = (sliders ? (RayTable[lsb_index(sliders)][ksq] & ~board.pieces(BOTH)) : ~board.pieces(BOTH));
            gen_quietproms(board, moveList, color, targets);
            gen_piece_quiets(board, moveList, color, targets);
            gen_capproms(board, moveList, color, checkers);
            gen_piece_caps(board, moveList, color, checkers);
            if (checkers & board.pieces(!color, PAWN)) {
                gen_ep(board, moveList, color, SQUARES[board.enPassant()]);
            }
            break;
        }
        default:
            assert(false);

    }

}

MoveList gen_legals(const Board& board, const MoveList& moves) {

    MoveList legals;

    for (unsigned int m = 0; m < moves.size; m++) {

        const Move move = moves.moves[m];

        if (board.is_legal(move)) {
            legals.append(move);
        }

    }

    return legals;

}

MoveList gen_quiets(const Board& board, const Color color) {

    MoveList moveList;

    if (board.checkers()) {
        gen_evasions(board, MOVES_QUIETS, moveList, color);
    } else {
        const uint64_t targets = ~board.pieces(BOTH);
        gen_quietproms(board, moveList, color, targets);
        gen_castlings(board, moveList, color);
        gen_piece_quiets(board, moveList, color, targets);
    }

    return moveList;

}

MoveList gen_caps(const Board& board, const Color color) {

    MoveList moveList;

    if (board.checkers()) {
        gen_evasions(board, MOVES_CAPTURES, moveList, color);
    } else {
        const uint64_t targets = board.pieces(!color);
        gen_capproms(board, moveList, color, targets);
        gen_piece_caps(board, moveList, color, targets);
        gen_ep(board, moveList, color, SQUARES[board.enPassant()]);
    }

    return moveList;

}

MoveList gen_all(const Board& board, const Color color) {

    MoveList moveList;

    moveList.merge(gen_quiets(board, color));
    moveList.merge(gen_caps(board, color));

    return moveList;

}

// Check if move is giving check
bool Board::gives_check(const Move move) {

    const unsigned ksq    = lsb_index(pieces(!stm, KING));
    const unsigned fromSq = from_sq(move);
    const unsigned toSq   = to_sq(move);

    uint64_t attacks = 0;

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

    // Check for discovered checks by moving the piece
    if (slider_attackers_discovered(stm, ksq, fromSq, toSq)) {
        return true;
    }

    return false;

}
