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

static const uint64_t WKCAS_BLOCKERS = 0x6;
static const uint64_t WQCAS_BLOCKERS = 0x70;
static const uint64_t BKCAS_BLOCKERS = 0x600000000000000;
static const uint64_t BQCAS_BLOCKERS = 0x7000000000000000;

static const uint64_t WKCAS_CHECKERS = 0x6;
static const uint64_t WQCAS_CHECKERS = 0x30;
static const uint64_t BKCAS_CHECKERS = 0x600000000000000;
static const uint64_t BQCAS_CHECKERS = 0x3000000000000000;

static const Move WKCAS_MOVE = make_move(60, 62, CASTLING);
static const Move WQCAS_MOVE = make_move(60, 58, CASTLING);
static const Move BKCAS_MOVE = make_move(4, 6, CASTLING);
static const Move BQCAS_MOVE = make_move(4, 2, CASTLING);

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

const Move MoveList::pick() {

    if (size == 0) return MOVE_NONE;

    int bestvalue = values[index];
    unsigned int bestindex = index;

    for (unsigned int mcount = index; mcount < size; mcount++) {
        if (values[mcount] > bestvalue) {
            bestvalue = values[mcount];
            bestindex = mcount;
        }
    }

    swap(index, bestindex);
    return moves[index];

}

void MoveList::print() {

    for (unsigned int mcount = 0; mcount < size; mcount++) {
        print_move(moves[mcount]);
    }

}

static void gen_quietproms(const Board& board, MoveList& moveList, const Color color, const uint64_t targets) {

    uint64_t pawns = board.pieces(PAWN, color) & PAWN_STARTRANK[!color];

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

    uint64_t pawns = board.pieces(PAWN, color) & PAWN_STARTRANK[!color];

    while (pawns) {

        const unsigned int fromsq = pop_lsb(pawns);
        uint64_t caps = AttackBitboards[Pawn(color)][fromsq] & targets;

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

    const unsigned int epsq = board.enPassant();

    if (epsq != NOSQ && (SQUARES[epsq] & targets)) {

        uint64_t pawns = AttackBitboards[Pawn(!color)][epsq] & board.pieces(PAWN, color);

        while (pawns) {

            moveList.append(make_move(pop_lsb(pawns), epsq, ENPASSANT));

        }

    }

}

static void gen_piece_caps(const Board& board, MoveList& moveList, const Color color, uint64_t targets) {

    const unsigned int sq = lsb_index(board.pieces(KING, color));
    uint64_t moves = AttackBitboards[KING][sq] & board.pieces(!color);

    while (moves) {

        moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

    }

    uint64_t pawns = board.pieces(PAWN, color) & ~PAWN_STARTRANK[!color];
    while (pawns) {

        const unsigned int sq = pop_lsb(pawns);
        uint64_t moves = AttackBitboards[Pawn(color)][sq] & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    uint64_t knights = board.pieces(KNIGHT, color);
    while (knights) {

        const unsigned int sq = pop_lsb(knights);
        uint64_t moves = AttackBitboards[KNIGHT][sq] & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    uint64_t bishops = board.pieces(BISHOP, color);
    while (bishops) {

        const unsigned int sq = pop_lsb(bishops);
        uint64_t moves = generateBishopMoves(sq, board.pieces(ALLPIECES), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    uint64_t rooks = board.pieces(ROOK, color);
    while (rooks) {

        const unsigned int sq = pop_lsb(rooks);
        uint64_t moves = generateRookMoves(sq, board.pieces(ALLPIECES), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }
    }

    uint64_t queens = board.pieces(QUEEN, color);
    while (queens) {

        const unsigned int sq = pop_lsb(queens);
        uint64_t moves = generateQueenMoves(sq, board.pieces(ALLPIECES), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

}

static void gen_piece_quiets(const Board& board, MoveList& moveList, const Color color, const uint64_t targets) {

    const uint64_t pawns      = board.pieces(PAWN, color) & ~PAWN_STARTRANK[!color];
    const uint64_t pawnPushes = shift_up(pawns, color) & ~board.pieces(ALLPIECES);
    uint64_t singlePushes     = pawnPushes & targets;
    uint64_t doublePushes     = shift_up(pawnPushes & PAWN_FIRST_PUSH_RANK[color], color) & targets;

    uint64_t knights = board.pieces(KNIGHT, color);
    while (knights) {

        const unsigned int sq = pop_lsb(knights);
        uint64_t moves = AttackBitboards[KNIGHT][sq] & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    uint64_t bishops = board.pieces(BISHOP, color);
    while (bishops) {

        const unsigned int sq = pop_lsb(bishops);
        uint64_t moves = generateBishopMoves(sq, board.pieces(ALLPIECES), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    uint64_t rooks = board.pieces(ROOK, color);
    while (rooks) {

        const unsigned int sq = pop_lsb(rooks);
        uint64_t moves = generateRookMoves(sq, board.pieces(ALLPIECES), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }
    }

    uint64_t queens = board.pieces(QUEEN, color);
    while (queens) {

        const unsigned int sq = pop_lsb(queens);
        uint64_t moves = generateQueenMoves(sq, board.pieces(ALLPIECES), board.pieces(color)) & targets;

        while (moves) {

            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

        }

    }

    while (singlePushes) {

        const unsigned int tosq = pop_lsb(singlePushes);
        const unsigned int fromsq = lsb_index((color == WHITE) ? (SQUARES[tosq] >> 8) : (SQUARES[tosq] << 8));
        moveList.append(make_move(fromsq, tosq, NORMAL));

    }

    while (doublePushes) {

        const unsigned int tosq = pop_lsb(doublePushes);
        const unsigned int fromsq = lsb_index((color == WHITE) ? (SQUARES[tosq] >> 16) : (SQUARES[tosq] << 16));
        moveList.append(make_move(fromsq, tosq, NORMAL));

    }

    const unsigned int sq = lsb_index(board.pieces(KING, color));
    uint64_t moves = AttackBitboards[KING][sq] & ~board.pieces(ALLPIECES);

    while (moves) {

        moveList.append(make_move(sq, pop_lsb(moves), NORMAL));

    }

}

bool Board::is_castling_valid(const unsigned int flag) const {

    if (!checkers()) {

        switch (flag) {

            case WKCASFLAG:
            {
                return (   state.castling & WKCASFLAG
                        && bitboards[WHITE_KING] & SQUARES[E1]
                        && bitboards[WHITE_ROOK] & SQUARES[H1]
                        && !getAllOccupancy(WKCAS_BLOCKERS));

            }
            case WQCASFLAG:
            {
                return (   state.castling & WQCASFLAG
                        && bitboards[WHITE_KING] & SQUARES[E1]
                        && bitboards[WHITE_ROOK] & SQUARES[A1]
                        && !getAllOccupancy(WQCAS_BLOCKERS));


            }
            case BKCASFLAG:
            {
                return (   state.castling & BKCASFLAG
                        && bitboards[BLACK_KING] & SQUARES[E8]
                        && bitboards[BLACK_ROOK] & SQUARES[H8]
                        && !getAllOccupancy(BKCAS_BLOCKERS));

            }
            case BQCASFLAG:
            {
                return (   state.castling & BQCASFLAG
                        && bitboards[BLACK_KING] & SQUARES[E8]
                        && bitboards[BLACK_ROOK] & SQUARES[A8]
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

static void gen_evasions(const Board& board, const MoveGenType mtype, MoveList& moveList, const Color color) {

    const unsigned int ksq  = lsb_index(board.pieces(KING, color));
    const uint64_t checkers = board.checkers();
    const uint64_t sliders  = checkers & ~(board.pieces(KNIGHT, color) | board.pieces(PAWN, color));

    if (popcount(checkers) >= 2) {

        uint64_t kingMoves = AttackBitboards[KING][ksq] & ((mtype == QUIETS) ? ~board.pieces(ALLPIECES) : board.pieces(!color));
        while (kingMoves) {
            moveList.append(make_move(ksq, pop_lsb(kingMoves), NORMAL));
        }
        return;

    }

    switch(mtype) {

        case QUIETS:
        {
            const uint64_t targets = (sliders ? (RayTable[lsb_index(sliders)][ksq] & ~board.pieces(ALLPIECES)) : ~board.pieces(ALLPIECES));
            gen_quietproms(board, moveList, color, targets);
            gen_piece_quiets(board, moveList, color, targets);
            break;
        }
        case CAPTURES:
        {
            gen_capproms(board, moveList, color, checkers);
            gen_piece_caps(board, moveList, color, checkers);
            if (checkers & board.pieces(PAWN, !color)) {
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
        gen_evasions(board, QUIETS, moveList, color);
    } else {
        const uint64_t targets = ~board.pieces(ALLPIECES);
        gen_quietproms(board, moveList, color, targets);
        gen_castlings(board, moveList, color);
        gen_piece_quiets(board, moveList, color, targets);
    }

    return moveList;

}

MoveList gen_caps(const Board& board, const Color color) {

    MoveList moveList;

    if (board.checkers()) {
        gen_evasions(board, CAPTURES, moveList, color);
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

    const unsigned int ksq    = lsb_index(bitboards[King(!stm)]);
    const unsigned int fromsq = from_sq(move);
    const unsigned int tosq   = to_sq(move);

    uint64_t attacks = 0;

    switch(type(piecetypes[fromsq])) {

        case PAWN:

            attacks = AttackBitboards[Pawn(stm)][tosq];
            break;

        case KNIGHT:

            attacks = AttackBitboards[KNIGHT][tosq];
            break;

        case BISHOP:

            attacks = generateBishopMoves(tosq, bitboards[ALLPIECES], bitboards[stm]);
            break;

        case ROOK:

            attacks = generateRookMoves(tosq, bitboards[ALLPIECES], bitboards[stm]);
            break;

        case QUEEN:

            attacks = generateQueenMoves(tosq, bitboards[ALLPIECES], bitboards[stm]);
            break;

        case KING:

            break;

        default:

            assert(false);

    }

    if (attacks & SQUARES[ksq]) {
        return true;
    }

    if (color_slider_attackers(stm, ksq, SQUARES[fromsq], SQUARES[tosq])) {
        return true;
    }

    return false;

}
