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

// Ranks
static const uint64_t RANK_1 = 0xFF;
static const uint64_t RANK_2 = RANK_1 << 8;
static const uint64_t RANK_3 = RANK_1 << 16;
static const uint64_t RANK_4 = RANK_1 << 24;
static const uint64_t RANK_5 = RANK_1 << 32;
static const uint64_t RANK_6 = RANK_1 << 40;
static const uint64_t RANK_7 = RANK_1 << 48;
static const uint64_t RANK_8 = RANK_1 << 56;

// Files
static const uint64_t FILE_H = 0x101010101010101;
static const uint64_t FILE_G = FILE_H << 1;
static const uint64_t FILE_F = FILE_H << 2;
static const uint64_t FILE_E = FILE_H << 3;
static const uint64_t FILE_D = FILE_H << 4;
static const uint64_t FILE_C = FILE_H << 5;
static const uint64_t FILE_B = FILE_H << 6;
static const uint64_t FILE_A = FILE_H << 7;

static const uint64_t RANKS[8] = { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };
static const uint64_t FILES[8] = { FILE_H, FILE_G, FILE_F, FILE_E, FILE_D, FILE_C, FILE_B, FILE_A };

// 64bit integers for each square
static const uint64_t SQUARES[65] = {

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

// Square names
enum {

    H1, G1, F1, E1, D1, C1, B1, A1,
    H2, G2, F2, E2, D2, C2, B2, A2,
    H3, G3, F3, E3, D3, C3, B3, A3,
    H4, G4, F4, E4, D4, C4, B4, A4,
    H5, G5, F5, E5, D5, C5, B5, A5,
    H6, G6, F6, E6, D6, C6, B6, A6,
    H7, G7, F7, E7, D7, C7, B7, A7,
    H8, G8, F8, E8, D8, C8, B8, A8,
    SQUARE_NONE

};

static const uint64_t WHITE_SQUARES = 0xaa55aa55aa55aa55;
static const uint64_t BLACK_SQUARES = 0x55aa55aa55aa55aa;
static const uint64_t ALL_SQUARES   = WHITE_SQUARES | BLACK_SQUARES;

#define WKCASFLAG 1
#define WQCASFLAG 2
#define BKCASFLAG 4
#define BQCASFLAG 8

#define WHITE_CASTLE_MASK  3
#define BLACK_CASTLE_MASK 12
#define ALL_CASTLE_MASK   15

static const uint64_t CASTLE_MASKS[2]   = { WHITE_CASTLE_MASK, BLACK_CASTLE_MASK };
static const uint64_t CASTLE_SQUARES[4] = { G1, C1, G8, C8 };
static const uint64_t CASTLE_FLAGS[4]   = { WKCASFLAG, WQCASFLAG, BKCASFLAG, BQCASFLAG };

typedef unsigned Direction;
typedef uint16_t Move;
typedef uint16_t MoveType;

enum MoveGenType : unsigned {

    QUIETS, CAPTURES

};

class Board;
class MoveList;

// Maximum Depth and Moves for search
#define MAX_DEPTH 100
#define MAX_MOVES 256

// Values for mate, draw, infinte, unknown
#define VALUE_MATE      50000
#define VALUE_MATE_MAX  (VALUE_MATE - MAX_DEPTH)
#define VALUE_MATED_MAX (MAX_DEPTH - VALUE_MATE)
#define VALUE_INFINITE  100000
#define VALUE_NONE      100001
#define VALUE_DRAW      0

#define DEPTH_NONE -36

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::high_resolution_clock::time_point TimePoint;

typedef struct {

    int mg = 0;
    int eg = 0;

} Value;

// Directions
enum {
    LEFT, UP, RIGHT, DOWN, LEFTUP, LEFTDOWN, RIGHTUP, RIGHTDOWN
};

static const int DIRECTIONS[2][8] = {

    { 1,   8, -1, -8,  9, -7,  7, -9 },
    { -1, -8,  1,  8, -9,  7, -7,  9 }

};

// Colors
enum Color : unsigned{
    WHITE, BLACK, BOTH
};

// Piecetypes
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

static const uint64_t CENTRAL_FILES = FILE_D | FILE_E;

static const uint64_t ADJ_FILES[8] = {

    FILE_G,
    FILE_F | FILE_H,
    FILE_E | FILE_G,
    FILE_D | FILE_F,
    FILE_C | FILE_E,
    FILE_B | FILE_D,
    FILE_A | FILE_C,
    FILE_B,

};

static const uint64_t KING_FLANK[8] = {

    FILE_F | FILE_G | FILE_H,
    FILE_E | FILE_F | FILE_G | FILE_H,
    FILE_E | FILE_F | FILE_G | FILE_H,
    FILE_C | FILE_D | FILE_E | FILE_F,
    FILE_C | FILE_D | FILE_E | FILE_F,
    FILE_A | FILE_B | FILE_C | FILE_D,
    FILE_A | FILE_B | FILE_C | FILE_D,
    FILE_A | FILE_B | FILE_C

};

static const uint64_t COLOUR_BASE_SQUARES[2] = {

    RANK_1 | RANK_2 | RANK_3, RANK_8 | RANK_7 | RANK_6

};

static const uint64_t CENTRAL_SQUARES = SQUARES[D4] | SQUARES[D5] | SQUARES[E4] | SQUARES[E5];

static const uint64_t PAWN_STARTRANK[2] = {

    RANK_2, RANK_7

};

static const uint64_t PAWN_FIRST_PUSH_RANK[2] = {

    RANK_3, RANK_6

};

static const uint64_t PAWN_FINALRANK[2] = {

    RANK_8, RANK_1

};

static std::map<unsigned int, unsigned int> castleByKingpos = { {G1, WKCASFLAG}, {C1, WQCASFLAG}, {G8, BKCASFLAG}, {C8, BQCASFLAG} };

// Get least significant set bit in unsigned 64bit integer
inline unsigned int lsb_index(const uint64_t bit) {

    return bitIndex64[((bit & -bit) * debruijn64) >> 58];

}

inline unsigned int msb_index(const uint64_t bit) {

    // __builtin_clzll returns undefined behaviour if bit is 0!

    return 63 - __builtin_clzll(bit);

}

// Get least significant set bit in unsigned 64bit integer
inline uint64_t lsb(const uint64_t bit) {

    return bit & -bit;

}

// Get most significant set bit in unsigned 64bit integer
inline uint64_t msb(const uint64_t bit) {

    // __builtin_clzll returns undefined behaviour if bit is 0!

    return SQUARES[63 - __builtin_clzll(bit)];

}

// Remove least significant set bit in unsigned 64bit integer
inline unsigned pop_lsb(uint64_t& bit) {

   const unsigned index = lsb_index(bit);
   bit ^= (1ull << index);
   return index;

}

// Count set bits in unsigned 64bit integer
inline int popcount(const uint64_t bit) {

    return __builtin_popcountll(bit);

}

// Check if a square is still on the board
inline bool sq_valid(const int sq) {

    return (sq >= 0 && sq < 64);

}

// Get rank index (0-7) of given square index
inline unsigned rank(const unsigned int sq) {

    return (sq >> 3);

}

// Get file index (0-7) of given square index
inline unsigned file(const unsigned int sq) {

    return (sq & 7);

}

inline unsigned square(const unsigned file, const unsigned rank) {

    return file + (rank * 8);

}

// Get relative rank index for given color of given square index
inline unsigned relative_rank(const Color color, const unsigned sq) {

    return (color == WHITE) ? rank(sq) : 7 - rank(sq);

}

inline unsigned relative_square(const Color color, const unsigned sq) {

    return (color == WHITE) ? sq : 63 - sq;

}

// Get most forward piece on bitboard for the given color
inline uint64_t most_forward(const Color color, const uint64_t bitboard) {

    return (color == WHITE) ? msb(bitboard) : lsb(bitboard);

}

// Get most backward piece on file for the given color
inline uint64_t most_backward(const Color color, const uint64_t bitboard) {

    return (color == WHITE) ? lsb(bitboard) : msb(bitboard);

}

inline uint64_t shift_up(const uint64_t b, const Color color) {

    return (color == WHITE) ? b << 8 : b >> 8;

}

inline uint64_t shift_down(const uint64_t b, const Color color) {

    return (color == WHITE) ? b >> 8 : b << 8;

}

inline uint64_t shift_left(const uint64_t b, const Color color) {

    return (color == WHITE) ? b << 1 : b >> 1;

}

inline uint64_t shift_right(const uint64_t b, const Color color) {

    return (color == WHITE) ? b >> 1 : b << 1;

}

inline Value V(const int mg, const int eg) {

    Value value;

    value.mg = mg;
    value.eg = eg;

    return value;

}

inline Value operator+=(Value& value, const Value value2) {

    value.mg += value2.mg;
    value.eg += value2.eg;
    return value;

}

inline Value operator-=(Value& value, const Value value2) {

    value.mg -= value2.mg;
    value.eg -= value2.eg;
    return value;

}

inline Value operator+(const Value value1, const Value value2) {

    Value value;
    value.mg = value1.mg + value2.mg;
    value.eg = value1.eg + value2.eg;
    return value;

}

inline Value operator-(const Value value1, const Value value2) {

    Value value;
    value.mg = value1.mg - value2.mg;
    value.eg = value1.eg - value2.eg;
    return value;

}

inline Value operator*(const Value value1, const int multiply) {

    Value value;
    value.mg = value1.mg * multiply;
    value.eg = value1.eg * multiply;
    return value;

}

inline Value operator*=(Value& value, const int multiply) {

    value.mg *= multiply;
    value.eg *= multiply;
    return value;

}

inline Value operator/(const Value value1, const int divisor) {

    Value value;
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

inline std::ostream& operator<<(std::ostream& os, const Value& value) {

    return os << "MG: " << value.mg << " | EG: " << value.eg;

}

#endif
