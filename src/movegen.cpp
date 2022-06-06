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

// Concatenates all moves of another list
void MoveList::concat(const MoveList& list) {

    std::copy(list.begin(), list.end(), moves.begin() + _size);
    _size += list.size();

}

// Swaps the position of two moves given their indices
void MoveList::swap(const unsigned index1, const unsigned index2) {

    std::iter_swap(moves.begin() + index1, moves.begin() + index2);

}

// Prints all moves in the list
void MoveList::print() {

    for (auto it = begin(); it != end(); ++it) {
        std::cout << move_to_string(*it) << std::endl;
    }

}

static void generate_pawn_promotions(MoveList& moveList, const Square fromSq, const Bitboard targets) {

    Bitboard squares = targets;
    while (squares) {

        const Square toSq = pop_lsb(squares);

        moveList.append(make_move(fromSq, toSq, PROMOTION_QUEEN));
        moveList.append(make_move(fromSq, toSq, PROMOTION_ROOK));
        moveList.append(make_move(fromSq, toSq, PROMOTION_BISHOP));
        moveList.append(make_move(fromSq, toSq, PROMOTION_KNIGHT));
    }

}

// Generates all pseudo-legal promotions in a given position and adds them to the given move list
template<MoveGenerationType T>
void generate_promotions(const Board& board, MoveList& moves, const Color color, const Bitboard targets) {

    static_assert(T != EVASION && T != ALL);

    Bitboard pawns = board.pieces(color, PAWN) & PAWN_STARTRANK[!color];

    while (pawns) {
        const Square fromSq = pop_lsb(pawns);

        if constexpr (T == QUIET) {
            generate_pawn_promotions(moves, fromSq, SQUARES[fromSq + direction(color, UP)] & targets);
        } else if constexpr (T == CAPTURE) {
            generate_pawn_promotions(moves, fromSq, PawnAttacks[color][fromSq] & targets);
        }
    }

}

// Generates the pseudo-legal en-passant captures (if possible) for a given position and adds them to the given move list
static void generate_enpassants(const Board& board, MoveList& moveList, const Color color, const Bitboard targets) {

    const Square epSq = board.enpassant_square();

    if (epSq != SQUARE_NONE && (SQUARES[epSq] & targets)) {

        // Bitboard of all friendly pawns attacking the en-passant square
        Bitboard pawns = PawnAttacks[!color][epSq] & board.pieces(color, PAWN);

        while (pawns) {
            moveList.append(make_move(pop_lsb(pawns), epSq, ENPASSANT));
        }

    }

}

template<MoveGenerationType T>
void generate_pawn_moves(const Board& board, MoveList& moves, const Color color, const Bitboard targets) {

    // Exclude pawns which are close to promotion
    // Promotions are handled by generate_promotions
    Bitboard pawns = board.pieces(color, PAWN) & ~PAWN_STARTRANK[!color];

    if constexpr (T == QUIET) {
        const Bitboard pawnPushes = shift_up(pawns, color) & ~board.pieces(BOTH);
        Bitboard singlePushes     = pawnPushes & targets;
        Bitboard doublePushes     = shift_up(pawnPushes & PAWN_FIRST_PUSH_RANK[color], color) & targets;

        while (singlePushes) {
            const Square toSq = pop_lsb(singlePushes);
            const Square fromSq = lsb_index(shift_down(SQUARES[toSq], color));
            moves.append(make_move(fromSq, toSq, NORMAL));
        }

        while (doublePushes) {
            const Square toSq = pop_lsb(doublePushes);
            const Square fromSq = lsb_index(shift_down(shift_down(SQUARES[toSq], color), color));
            moves.append(make_move(fromSq, toSq, NORMAL));
        }
    } else if constexpr (T == CAPTURE) {
        while (pawns) {
            const Square sq  = pop_lsb(pawns);
            Bitboard attacks = PawnAttacks[color][sq] & targets;

            while (attacks) {
                moves.append(make_move(sq, pop_lsb(attacks), NORMAL));
            }
        }
    }

}

static void generate_knight_moves(const Board& board, MoveList& moveList, Color color, Bitboard targets) {

    Bitboard knights = board.pieces(color, KNIGHT);
    while (knights) {

        const Square sq = pop_lsb(knights);
        Bitboard moves = piece_attacks<KNIGHT>(sq) & targets;

        while (moves) {
            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));
        }

    }

}

static void generate_bishop_moves(const Board& board, MoveList& moveList, Color color, Bitboard targets) {

    Bitboard bishops = board.pieces(color, BISHOP);
    while (bishops) {

        const Square sq = pop_lsb(bishops);
        Bitboard moves = piece_attacks<BISHOP>(sq, board.pieces(BOTH)) & targets;

        while (moves) {
            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));
        }

    }

}

static void generate_rook_moves(const Board& board, MoveList& moveList, Color color, Bitboard targets) {

    Bitboard rooks = board.pieces(color, ROOK);
    while (rooks) {

        const Square sq = pop_lsb(rooks);
        Bitboard moves = piece_attacks<ROOK>(sq, board.pieces(BOTH)) & targets;

        while (moves) {
            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));
        }

    }

}

static void generate_queen_moves(const Board& board, MoveList& moveList, Color color, Bitboard targets) {

    Bitboard queens = board.pieces(color, QUEEN);
    while (queens) {

        const Square sq = pop_lsb(queens);
        Bitboard moves = piece_attacks<QUEEN>(sq, board.pieces(BOTH)) & targets;

        while (moves) {
            moveList.append(make_move(sq, pop_lsb(moves), NORMAL));
        }

    }

}

static void generate_king_moves(const Board& board, MoveList& moveList, Color color, Bitboard targets) {

    const Square sq = lsb_index(board.pieces(color, KING));
    Bitboard moves = piece_attacks<KING>(sq) & targets;

    while (moves) {
        moveList.append(make_move(sq, pop_lsb(moves), NORMAL));
    }

}

// Generates all pseudo-legal moves capturing pieces for a given color and adds them to the move list
// For each piece, the method creates a bitboard with all possible target squares. It then loops over each
// set bit on the bitboard and adds a move for each to the move list until there are not bits left on the bitboard.
static void generate_captures(const Board& board, MoveList& moveList, const Color color, Bitboard targets) {

    generate_king_moves(board, moveList, color, board.pieces(!color));

    generate_pawn_moves<CAPTURE>(board, moveList, color, targets);

    generate_knight_moves(board, moveList, color, targets);
    generate_bishop_moves(board, moveList, color, targets);
    generate_rook_moves(board, moveList, color, targets);
    generate_queen_moves(board, moveList, color, targets);

}

// Same as the function above, only for quiet moves though
static void generate_quiets(const Board& board, MoveList& moveList, const Color color, const Bitboard targets) {

    generate_knight_moves(board, moveList, color, targets);
    generate_bishop_moves(board, moveList, color, targets);
    generate_rook_moves(board, moveList, color, targets);
    generate_queen_moves(board, moveList, color, targets);

    generate_pawn_moves<QUIET>(board, moveList, color, targets);

    generate_king_moves(board, moveList, color, ~board.pieces(BOTH));

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
static void generate_castlings(const Board& board, MoveList& moveList, const Color color) {

    if (board.is_castling_valid(CASTLE_SHORT)) {
        moveList.append(CASTLE_MOVES[color][CASTLE_SHORT]);
    }

    if (board.is_castling_valid(CASTLE_LONG)) {
        moveList.append(CASTLE_MOVES[color][CASTLE_LONG]);
    }

}

// Given a move list and a board, this function returns a new move list
// containing only the legal moves of the original list
static MoveList filter_legals(const Board& board, const MoveList& moves) {

    MoveList legals;

    for (const Move& move : moves) {
        if (board.is_legal(move)) {
            legals.append(move);
        }
    }

    return legals;

}

template<MoveGenerationType T, MoveLegality L>
MoveList generate_moves(const Board& board, const Color color);

// Generates all possible pseudo-legal quiet moves in the given position for a given color
template<>
MoveList generate_moves<QUIET, PSEUDO_LEGAL>(const Board& board, const Color color) {

    MoveList moveList;

    const Bitboard targets = board.empty_squares();

    generate_promotions<QUIET>(board, moveList, color, targets);
    generate_castlings(board, moveList, color);
    generate_quiets(board, moveList, color, targets);

    return moveList;

}

// Generates all possible pseudo-legal moves capturing a piece in the given position for a given color
template<>
MoveList generate_moves<CAPTURE, PSEUDO_LEGAL>(const Board& board, const Color color) {

    MoveList moveList;

    const Bitboard targets = board.pieces(!color);

    generate_promotions<CAPTURE>(board, moveList, color, targets);
    generate_captures(board, moveList, color, targets);
    generate_enpassants(board, moveList, color, SQUARES[board.enpassant_square()]);

    return moveList;

}

// Generates all possible moves evading a check for the given color in the given position and adds all evasions to the move list
template<>
MoveList generate_moves<EVASION, PSEUDO_LEGAL>(const Board& board, const Color color) {

    assert(board.checkers());
    assert(color == board.turn());

    MoveList moves;

    if (popcount(board.checkers()) >= 2) {
        generate_king_moves(board, moves, color, ~board.pieces(color));
        return moves;
    }

    const Square ksq        = lsb_index(board.pieces(color, KING));
    const Bitboard checkers = board.checkers();
    // TODO: This looks wrong but still works:
    // This still includes opposing knights/pawns
    const Bitboard sliders  = checkers & ~(board.pieces(color, KNIGHT) | board.pieces(color, PAWN));

    const Bitboard targets = (sliders ? (RayTable[lsb_index(sliders)][ksq] & ~board.pieces(BOTH)) : ~board.pieces(BOTH));
    generate_promotions<QUIET>(board, moves, color, targets);
    generate_quiets(board, moves, color, targets);
    generate_promotions<CAPTURE>(board, moves, color, checkers);
    generate_captures(board, moves, color, checkers);
    if (checkers & board.pieces(!color, PAWN)) {
        generate_enpassants(board, moves, color, SQUARES[board.enpassant_square()]);
    }

    return moves;

}

// Generates all possible pseudo-legal moves for the given position for a given color
template<>
MoveList generate_moves<ALL, PSEUDO_LEGAL>(const Board& board, const Color color) {

    MoveList moves;

    moves.concat(generate_moves<QUIET, PSEUDO_LEGAL>(board, color));
    moves.concat(generate_moves<CAPTURE, PSEUDO_LEGAL>(board, color));

    return moves;

}

// Generates all legal moves for the given position for a given color
template<>
MoveList generate_moves<ALL, LEGAL>(const Board& board, const Color color) {

    return filter_legals(board, generate_moves<ALL, PSEUDO_LEGAL>(board, color));

}

// Checks wether a move is giving check to the opponent's king
bool Board::gives_check(const Move move) {

    const Square fromSq = from_sq(move);
    const Square toSq   = to_sq(move);
    Piecetype pt = pieceTypes[fromSq];

    // Check if the piece attacks the opponents king directly
    if (SQUARES[toSq] & check_squares(pt)) {
        return true;
    }

    // Check for discovered checks from sliders which appear by moving the piece
    if (slider_attackers_discovered(stm, king_square(!stm), fromSq, toSq)) {
        return true;
    }

    return false;

}
