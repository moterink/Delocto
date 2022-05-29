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

#ifndef TYPES_H
#define TYPES_H

#include <iostream>
#include <map>
#include <algorithm>
#include <cstdint>
#include <vector>
#include <string>
#include <cassert>
#include <functional>
#include <complex>
#include <array>
#include <iomanip>
#include <utility>
#include <cmath>
#include <chrono>

#define VERSION 0.6

// Indices for ranks
enum Rank : int {

    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8

};

// Indices for files
enum File : int {

    FILE_H, FILE_G, FILE_F, FILE_E, FILE_D, FILE_C, FILE_B, FILE_A

};


typedef uint64_t Bitboard;

// Bitboards for ranks
constexpr Bitboard BB_RANK_1 = 0xFF;
constexpr Bitboard BB_RANK_2 = BB_RANK_1 << 8;
constexpr Bitboard BB_RANK_3 = BB_RANK_1 << 16;
constexpr Bitboard BB_RANK_4 = BB_RANK_1 << 24;
constexpr Bitboard BB_RANK_5 = BB_RANK_1 << 32;
constexpr Bitboard BB_RANK_6 = BB_RANK_1 << 40;
constexpr Bitboard BB_RANK_7 = BB_RANK_1 << 48;
constexpr Bitboard BB_RANK_8 = BB_RANK_1 << 56;

// Bitboards for files
constexpr Bitboard BB_FILE_H = 0x101010101010101;
constexpr Bitboard BB_FILE_G = BB_FILE_H << 1;
constexpr Bitboard BB_FILE_F = BB_FILE_H << 2;
constexpr Bitboard BB_FILE_E = BB_FILE_H << 3;
constexpr Bitboard BB_FILE_D = BB_FILE_H << 4;
constexpr Bitboard BB_FILE_C = BB_FILE_H << 5;
constexpr Bitboard BB_FILE_B = BB_FILE_H << 6;
constexpr Bitboard BB_FILE_A = BB_FILE_H << 7;

// Arrays of bitboards for ranks and files so they can be accessed numerically
constexpr Bitboard RANKS[8] = { BB_RANK_1, BB_RANK_2, BB_RANK_3, BB_RANK_4, BB_RANK_5, BB_RANK_6, BB_RANK_7, BB_RANK_8 };
constexpr Bitboard FILES[8] = { BB_FILE_H, BB_FILE_G, BB_FILE_F, BB_FILE_E, BB_FILE_D, BB_FILE_C, BB_FILE_B, BB_FILE_A };

// Bitboards for each square
constexpr Bitboard SQUARES[65] = {

    0x1,               0x2,               0x4,               0x8,               0x10,               0x20,               0x40,               0x80,
    0x100,             0x200,             0x400,             0x800,             0x1000,             0x2000,             0x4000,             0x8000,
    0x10000,           0x20000,           0x40000,           0x80000,           0x100000,           0x200000,           0x400000,           0x800000,
    0x1000000,         0x2000000,         0x4000000,         0x8000000,         0x10000000,         0x20000000,         0x40000000,         0x80000000,
    0x100000000,       0x200000000,       0x400000000,       0x800000000,       0x1000000000,       0x2000000000,       0x4000000000,       0x8000000000,
    0x10000000000,     0x20000000000,     0x40000000000,     0x80000000000,     0x100000000000,     0x200000000000,     0x400000000000,     0x800000000000,
    0x1000000000000,   0x2000000000000,   0x4000000000000,   0x8000000000000,   0x10000000000000,   0x20000000000000,   0x40000000000000,   0x80000000000000,
    0x100000000000000, 0x200000000000000, 0x400000000000000, 0x800000000000000, 0x1000000000000000, 0x2000000000000000, 0x4000000000000000, 0x8000000000000000,
    0x0

};

// Indices for each square, SQUARE_NONE = 64
constexpr unsigned SQUARE_COUNT = 64;

typedef int Square;

enum : int {

    SQUARE_H1, SQUARE_G1, SQUARE_F1, SQUARE_E1, SQUARE_D1, SQUARE_C1, SQUARE_B1, SQUARE_A1,
    SQUARE_H2, SQUARE_G2, SQUARE_F2, SQUARE_E2, SQUARE_D2, SQUARE_C2, SQUARE_B2, SQUARE_A2,
    SQUARE_H3, SQUARE_G3, SQUARE_F3, SQUARE_E3, SQUARE_D3, SQUARE_C3, SQUARE_B3, SQUARE_A3,
    SQUARE_H4, SQUARE_G4, SQUARE_F4, SQUARE_E4, SQUARE_D4, SQUARE_C4, SQUARE_B4, SQUARE_A4,
    SQUARE_H5, SQUARE_G5, SQUARE_F5, SQUARE_E5, SQUARE_D5, SQUARE_C5, SQUARE_B5, SQUARE_A5,
    SQUARE_H6, SQUARE_G6, SQUARE_F6, SQUARE_E6, SQUARE_D6, SQUARE_C6, SQUARE_B6, SQUARE_A6,
    SQUARE_H7, SQUARE_G7, SQUARE_F7, SQUARE_E7, SQUARE_D7, SQUARE_C7, SQUARE_B7, SQUARE_A7,
    SQUARE_H8, SQUARE_G8, SQUARE_F8, SQUARE_E8, SQUARE_D8, SQUARE_C8, SQUARE_B8, SQUARE_A8,
    SQUARE_NONE

};

// Bitboards for white, black and all squares on the board
constexpr Bitboard SQUARES_WHITE = 0xaa55aa55aa55aa55;
constexpr Bitboard SQUARES_BLACK = 0x55aa55aa55aa55aa;
constexpr Bitboard SQUARES_ALL   = SQUARES_WHITE | SQUARES_BLACK;

typedef uint16_t Move;
typedef uint16_t MoveType;

enum MoveGenerationType : unsigned {

    QUIET, CAPTURE, EVASION, ALL

};

enum MoveLegality : unsigned {

    PSEUDO_LEGAL, LEGAL

};

class Board;
class MoveList;

typedef int Value;
typedef int Depth;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::high_resolution_clock::time_point TimePoint;
typedef long long Duration;

// Maximum depth and moves for search
constexpr Depth DEPTH_MAX    = 100;
constexpr Depth DEPTH_NONE   = -36;

// Values for mate, draw, infinte, unknown
// They need to be equal or below the numeric limit of int16_t,
// because mate values need to fit into a transposition table entry
constexpr Value VALUE_NONE      = 0x7FFF;
constexpr Value VALUE_INFINITE  = VALUE_NONE - 1;
constexpr Value VALUE_MATE      = VALUE_INFINITE - 1;
constexpr Value VALUE_MATE_MAX  =  VALUE_MATE - DEPTH_MAX;
constexpr Value VALUE_MATED_MAX = -VALUE_MATE + DEPTH_MAX;
constexpr Value VALUE_DRAW      = 0;

// EvalTerm structure
// Consists of a value for the midgame and one for the endgame
struct EvalTerm {

    int mg = 0;
    int eg = 0;

};

// Directions
enum : int {
    LEFT      =  1,
    UP        =  8,
    RIGHT     = -1,
    DOWN      = -8,
    LEFTUP    =  9,
    LEFTDOWN  = -7,
    RIGHTUP   =  7,
    RIGHTDOWN = -9
};

// Colors
constexpr unsigned COLOR_COUNT = 2;

enum Color : unsigned {
    WHITE, BLACK, BOTH
};

// Piecetypes
constexpr unsigned PIECETYPE_COUNT = 6;

enum Piecetype : unsigned {
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, PIECE_NONE
};

// Lookup table for debruijn-index
static const int bitIndex64[64] = {

    0,  1, 48,  2, 57, 49, 28,  3,
   61, 58, 50, 42, 38, 29, 17,  4,
   62, 55, 59, 36, 53, 51, 43, 22,
   45, 39, 33, 30, 24, 18, 12,  5,
   63, 47, 56, 27, 60, 41, 37, 16,
   54, 35, 52, 21, 44, 32, 23, 11,
   46, 26, 40, 15, 34, 20, 31, 10,
   25, 14, 19,  9, 13,  8,  7,  6

};

static const uint64_t debruijn64 = 0x03f79d71b4cb0a89;

// Central files bitboards on the board
constexpr Bitboard CENTRAL_FILES = BB_FILE_D | BB_FILE_E;

// Adjacent files bitboards array, accessed by file index
constexpr Bitboard ADJ_FILES[8] = {

    BB_FILE_G,
    BB_FILE_F | BB_FILE_H,
    BB_FILE_E | BB_FILE_G,
    BB_FILE_D | BB_FILE_F,
    BB_FILE_C | BB_FILE_E,
    BB_FILE_B | BB_FILE_D,
    BB_FILE_A | BB_FILE_C,
    BB_FILE_B,

};

// Bitboards for the king flank given a file index
constexpr Bitboard KING_FLANK[8] = {

    BB_FILE_F | BB_FILE_G | BB_FILE_H,
    BB_FILE_E | BB_FILE_F | BB_FILE_G | BB_FILE_H,
    BB_FILE_E | BB_FILE_F | BB_FILE_G | BB_FILE_H,
    BB_FILE_C | BB_FILE_D | BB_FILE_E | BB_FILE_F,
    BB_FILE_C | BB_FILE_D | BB_FILE_E | BB_FILE_F,
    BB_FILE_A | BB_FILE_B | BB_FILE_C | BB_FILE_D,
    BB_FILE_A | BB_FILE_B | BB_FILE_C | BB_FILE_D,
    BB_FILE_A | BB_FILE_B | BB_FILE_C

};

// Base ranks (first 3 ranks) for a given color
constexpr Bitboard COLOR_BASE_RANKS[COLOR_COUNT] = {

    BB_RANK_1 | BB_RANK_2 | BB_RANK_3, BB_RANK_8 | BB_RANK_7 | BB_RANK_6

};

// 4 central squares bitboard (D4, D5, E4, E5)
constexpr Bitboard CENTRAL_SQUARES = SQUARES[SQUARE_D4] | SQUARES[SQUARE_D5] | SQUARES[SQUARE_E4] | SQUARES[SQUARE_E5];

// Start rank bitboards of the pawns
constexpr Bitboard PAWN_STARTRANK[COLOR_COUNT] = {

    BB_RANK_2, BB_RANK_7

};

// Bitboards for the first rank pawns can reach
constexpr Bitboard PAWN_FIRST_PUSH_RANK[COLOR_COUNT] = {

    BB_RANK_3, BB_RANK_6

};

// Bitboards for the ranks pawns have to reach in order to be promoted
constexpr Bitboard PAWN_FINALRANK[COLOR_COUNT] = {

    BB_RANK_8, BB_RANK_1

};

// Get least significant set bit in unsigned 64bit integer
inline Square lsb_index(const Bitboard b) {

    return static_cast<Square>(bitIndex64[((b & -b) * debruijn64) >> 58]);

}

inline Square msb_index(const Bitboard bit) {

    // __builtin_clzll returns undefined behaviour if bit is 0!

    return static_cast<Square>(63 - __builtin_clzll(bit));

}

// Get least significant set bit in unsigned 64bit integer
inline Bitboard lsb(const Bitboard b) {

    return b & -b;

}

// Get most significant set bit in unsigned 64bit integer
inline uint64_t msb(const Bitboard b) {

    // __builtin_clzll returns undefined behaviour if bit is 0!
    assert(b != 0);

    return SQUARES[63 - __builtin_clzll(b)];

}

// Remove least significant set bit in unsigned 64bit integer
inline Square pop_lsb(Bitboard& bit) {

   const Square sq = lsb_index(bit);
   bit ^= (1ull << sq);
   return sq;

}

// Count set bits in unsigned 64bit integer
inline int popcount(const Bitboard b) {

    return __builtin_popcountll(b);

}

// Check if a square is still on the board
inline bool sq_valid(const Square sq) {

    return (sq >= 0 && sq < 64);

}

// Get rank index (0-7) of given square index
inline Rank rank(const Square sq) {

    return Rank(sq >> 3);

}

// Get file index (0-7) of given square index
inline File file(const Square sq) {

    return File(sq & 7);

}

inline Square square(const unsigned file, const unsigned rank) {

    return static_cast<Square>(file + (rank * 8));

}

// Get relative rank index for given color of given square index
inline Rank relative_rank(const Color color, const Square sq) {

    return Rank((color == WHITE) ? rank(sq) : 7 - rank(sq));

}

inline Square relative_square(const Color color, const Square sq) {

    return static_cast<Square>(color == WHITE ? sq : 63 - sq);

}

// Get most forward piece on bitboard for the given color
inline Bitboard most_forward(const Color color, const Bitboard bitboard) {

    return (color == WHITE) ? msb(bitboard) : lsb(bitboard);

}

// Get most backward piece on bitboard for the given color
inline Bitboard most_backward(const Color color, const Bitboard bitboard) {

    return (color == WHITE) ? lsb(bitboard) : msb(bitboard);

}

inline const int direction(const Color color, const int direction) {
    return direction * (color == WHITE ? 1 : -1);
}

inline Bitboard shift_up(const Bitboard b, const Color color) {

    return (color == WHITE) ? b << 8 : b >> 8;

}

inline Bitboard shift_down(const Bitboard b, const Color color) {

    return (color == WHITE) ? b >> 8 : b << 8;

}

inline Bitboard shift_left(const Bitboard b, const Color color) {

    return (color == WHITE) ? b << 1 : b >> 1;

}

inline Bitboard shift_right(const Bitboard b, const Color color) {

    return (color == WHITE) ? b >> 1 : b << 1;

}

inline EvalTerm V(const Value mg, const Value eg) {

    EvalTerm value;

    value.mg = mg;
    value.eg = eg;

    return value;

}

inline bool operator==(const EvalTerm& value1, const EvalTerm& value2) {
    return value1.mg == value2.mg && value1.eg == value2.eg;
}

inline EvalTerm operator+=(EvalTerm& value, const EvalTerm value2) {

    value.mg += value2.mg;
    value.eg += value2.eg;
    return value;

}

inline EvalTerm operator-=(EvalTerm& value, const EvalTerm value2) {

    value.mg -= value2.mg;
    value.eg -= value2.eg;
    return value;

}

inline EvalTerm operator+(const EvalTerm value1, const EvalTerm value2) {

    EvalTerm value;
    value.mg = value1.mg + value2.mg;
    value.eg = value1.eg + value2.eg;
    return value;

}

inline EvalTerm operator-(const EvalTerm value1, const EvalTerm value2) {

    EvalTerm value;
    value.mg = value1.mg - value2.mg;
    value.eg = value1.eg - value2.eg;
    return value;

}

inline EvalTerm operator*(const EvalTerm value1, const int multiply) {

    EvalTerm value;
    value.mg = value1.mg * multiply;
    value.eg = value1.eg * multiply;
    return value;

}

inline EvalTerm operator*=(EvalTerm& value, const int multiply) {

    value.mg *= multiply;
    value.eg *= multiply;
    return value;

}

inline EvalTerm operator/(const EvalTerm value1, const int divisor) {

    EvalTerm value;
    value.mg = value1.mg / divisor;
    value.eg = value1.eg / divisor;
    return value;

}

inline Color operator!(const Color color) {

    return Color(!unsigned(color));

}

inline Color& operator++(Color& c, int) {

    return c = Color(int(c) + 1);

}

inline Piecetype& operator++(Piecetype& pt, int) {

    return pt = Piecetype(int(pt) + 1);

}

// Print a value to the console, showing the midgame and endgame terms
inline std::ostream& operator<<(std::ostream& os, const EvalTerm& value) {

    return os << "MG: " << value.mg << " | EG: " << value.eg;

}

#endif
