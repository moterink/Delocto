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

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.hpp"
#include "move.hpp"
#include "bitboards.hpp"

// Maximum number of possible moves ever in a chess position
constexpr unsigned MOVES_MAX_COUNT = 218;

class MoveList {

    public:

        inline void append(Move move) {
            moves[_size++] = move;
        }

        Move operator [](unsigned index) const { return moves[index]; }

        std::array<Move, MOVES_MAX_COUNT>::const_iterator begin() const { return moves.begin(); }
        std::array<Move, MOVES_MAX_COUNT>::const_iterator end() const { return moves.begin() + _size; }

        void concat(const MoveList& list);
        unsigned find(const Move move);
        void swap(const unsigned index1, const unsigned index2);
        void print();

        unsigned size() const { return _size; }

    protected:

        unsigned _size = 0;

        std::array<Move, MOVES_MAX_COUNT> moves;

};

// Flags for the castling rights
typedef unsigned CastleRight;

constexpr CastleRight CASTLE_NONE = 0x0;
constexpr CastleRight CASTLE_WHITE_SHORT = 0x1;
constexpr CastleRight CASTLE_WHITE_LONG  = CASTLE_WHITE_SHORT << 1;
constexpr CastleRight CASTLE_BLACK_SHORT = CASTLE_WHITE_SHORT << 2;
constexpr CastleRight CASTLE_BLACK_LONG  = CASTLE_WHITE_SHORT << 3;

constexpr unsigned CASTLE_SHORT = 0;
constexpr unsigned CASTLE_LONG  = 1;

constexpr CastleRight CASTLE_TYPES[COLOR_COUNT][2] = {
    { CASTLE_WHITE_SHORT, CASTLE_WHITE_LONG },
    { CASTLE_BLACK_SHORT, CASTLE_BLACK_LONG }
};

constexpr unsigned CASTLE_KING_TARGET_SQUARE[COLOR_COUNT][2] = { { SQUARE_G1, SQUARE_C1 }, { SQUARE_G8, SQUARE_C8 } };
constexpr unsigned CASTLE_ROOK_ORIGIN_SQUARE[COLOR_COUNT][2] = { { SQUARE_H1, SQUARE_A1 }, { SQUARE_H8, SQUARE_A8 } };

constexpr unsigned KING_INITIAL_SQUARE[COLOR_COUNT] = { SQUARE_E1, SQUARE_E8 };

// Blocking squares for the castling moves
constexpr Bitboard CASTLE_PATH[COLOR_COUNT][2] = {
    {
        SQUARES[SQUARE_F1] | SQUARES[SQUARE_G1],
        SQUARES[SQUARE_B1] | SQUARES[SQUARE_C1] | SQUARES[SQUARE_D1]
    },
    {
        SQUARES[SQUARE_F8] | SQUARES[SQUARE_G8],
        SQUARES[SQUARE_B8] | SQUARES[SQUARE_C8] | SQUARES[SQUARE_D8]
    }
};

// Move objects for each castling
const Move CASTLE_MOVES[COLOR_COUNT][2] = {
    {
        make_move(SQUARE_E1, SQUARE_G1, CASTLING),
        make_move(SQUARE_E1, SQUARE_C1, CASTLING)
    },
    {
        make_move(SQUARE_E8, SQUARE_G8, CASTLING),
        make_move(SQUARE_E8, SQUARE_C8, CASTLING)
    }
};

// Pawns
// Returns the index of the single push square for a pawn on the given square for the given color
inline unsigned get_pawn_push_sq(const unsigned sq, const Bitboard allPieces, const Square up) {

    return (sq_valid(sq + up) && (!(SQUARES[sq + up] & allPieces))) ? sq + up : SQUARE_NONE;

}

// Returns the index of the double push square for a pawn on the given square for the given color
inline unsigned get_pawn_double_push_sq(const unsigned sq, const Bitboard allPieces, const int up, const Color color) {

    return (sq_valid(sq + (2 * up)) && (SQUARES[sq] & PAWN_STARTRANK[color]) && (!((SQUARES[sq + (2 * up)]) & allPieces)) && (!(SQUARES[sq + up] & allPieces))) ? sq + (2 * up) : SQUARE_NONE;

}

// Returns a bitboard of all squares a pawn on the given square can capture pieces for the given color
inline Bitboard generate_pawn_captures(const unsigned sq, const Bitboard oppPieces, const Color color) {

    return PawnAttacks[color][sq] & oppPieces;

}

// Returns a bitboard of all possible pawn destination squares on the given square for the given color
inline Bitboard generate_pawn_moves(const Color color, const unsigned sq, const Bitboard allPieces, const Bitboard oppPieces) {

    return (SQUARES[get_pawn_push_sq(sq, allPieces, direction(color, UP))]
          | SQUARES[get_pawn_double_push_sq(sq, allPieces, direction(color, UP), color)]
          | generate_pawn_captures(sq, oppPieces, color));

}

// Returns a bitboard of all attack squares for a pawn on the given square for the given color
inline Bitboard generate_pawns_attacks(const Bitboard pawns, const Color color) {

	return (color == WHITE) ? (((pawns & ~BB_FILE_A) << 9) | ((pawns & ~BB_FILE_H) << 7)) : (((pawns & ~BB_FILE_A) >> 7) | ((pawns & ~BB_FILE_H) >> 9));

}

// Returns a bitboard of all possible destination squares for a knight on the given square
inline Bitboard knight_target_squares(const unsigned sq, const Bitboard ownPieces) {

    return KnightAttacks[sq] & ~ownPieces;

}

// Returns a bitboard of all possible destination squares for a king on the given square
inline Bitboard king_target_squares(const unsigned sq, const Bitboard ownPieces) {

    return KingAttacks[sq] & ~ownPieces;

}

// Sliding Pieces
// Returns a bitboard of all possible destination squares for a bishop on the given square
inline Bitboard bishop_target_squares(const Square sq, const Bitboard both, const Bitboard friendly) {

    return BishopMagics[sq].attacks[get_magic_index(both, &BishopMagics[sq])] & ~friendly;

}

// Returns a bitboard of all possible destination squares for a rook on the given square
inline Bitboard rook_target_squares(const Square sq, const Bitboard both, const Bitboard friendly) {

    return RookMagics[sq].attacks[get_magic_index(both, &RookMagics[sq])] & ~friendly;

}

// Returns a bitboard of all possible destination squares for a queen on the given square
inline Bitboard queen_target_squares(const Square sq, const Bitboard both, const Bitboard friendly) {

    return bishop_target_squares(sq, both, friendly) | rook_target_squares(sq, both, friendly);

}

template<MoveGenerationType T, MoveLegality L>
extern MoveList generate_moves(const Board& board, const Color color);

#endif
