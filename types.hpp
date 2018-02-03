/*
  Delocto Chess Engine
  Copyright (c) 2018 Moritz Terink

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

#ifndef GENS_H
#define GENS_H

#define NDEBUG

#ifdef __APPLE__
    #define TIME_DIVISOR (CLOCKS_PER_SEC / 1000)
#else
    #define TIME_DIVISOR (CLOCKS_PER_SEC * 1000)
#endif

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

// Ranks
static const uint64_t RANK_8 = 0xFF;
static const uint64_t RANK_7 = RANK_8 << 8;
static const uint64_t RANK_6 = RANK_8 << 16;
static const uint64_t RANK_5 = RANK_8 << 24;
static const uint64_t RANK_4 = RANK_8 << 32;
static const uint64_t RANK_3 = RANK_8 << 40;
static const uint64_t RANK_2 = RANK_8 << 48;
static const uint64_t RANK_1 = RANK_8 << 56;

// Files
static const uint64_t FILE_H = 0x101010101010101;
static const uint64_t FILE_G = FILE_H << 1;
static const uint64_t FILE_F = FILE_H << 2;
static const uint64_t FILE_E = FILE_H << 3;
static const uint64_t FILE_D = FILE_H << 4;
static const uint64_t FILE_C = FILE_H << 5;
static const uint64_t FILE_B = FILE_H << 6;
static const uint64_t FILE_A = FILE_H << 7;

static const uint64_t WHITE_SQUARES = 0xaa55aa55aa55aa55;
static const uint64_t BLACK_SQUARES = 0x55aa55aa55aa55aa;
static const uint64_t ALL_SQUARES   = WHITE_SQUARES | BLACK_SQUARES;

static const uint64_t RANKS[8] = { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };
static const uint64_t FILES[8] = { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };

#define NOSQ 64

#define WKCASFLAG 1
#define WQCASFLAG 2
#define BKCASFLAG 4
#define BQCASFLAG 8

#define WHITE_CASTLE_MASK  3
#define BLACK_CASTLE_MASK 12
#define ALL_CASTLE_MASK   15

static const uint64_t CASTLE_MASKS[2] = { WHITE_CASTLE_MASK, BLACK_CASTLE_MASK };

typedef unsigned int Side;
typedef unsigned int PieceType;
typedef unsigned int Direction;
typedef unsigned int CastlePerm;
typedef uint16_t Move;
typedef uint16_t MoveType;

enum MoveGenType : unsigned int {
    
    QUIETS, CAPTURES
    
};

class Board;
class MoveList;

// Scores for mate, draw, infinte, unknown
#define MATEVALUE     50000
#define MINMATE       49000
#define INFINITE     100000
#define UNKNOWNVALUE 100001
#define DRAWVALUE         0

typedef struct {
    
    int mg = 0;
    int eg = 0;
    
}Score;

// Directions
enum {
    
    LEFT, UP, RIGHT, DOWN, LEFTUP, LEFTDOWN, RIGHTUP, RIGHTDOWN
    
};

// Sides
#define WHITE 0
#define BLACK 1

// Piecetypes
#define PAWN       2
#define KNIGHT     4
#define BISHOP     6
#define ROOK       8
#define QUEEN     10
#define KING      12
#define NOPIECE   14
#define ALLPIECES 15

// Piecetypes are defined such that a simple AND can show side and general type

// White piecetypes
#define WHITE_PAWN    2
#define WHITE_KNIGHT  4
#define WHITE_BISHOP  6
#define WHITE_ROOK    8
#define WHITE_QUEEN  10
#define WHITE_KING   12

// Black piecetypes
#define BLACK_PAWN    3
#define BLACK_KNIGHT  5
#define BLACK_BISHOP  7
#define BLACK_ROOK    9
#define BLACK_QUEEN  11
#define BLACK_KING   13

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

static const uint64_t ADJ_FILES[8] = {
    
    FILE_B,
    FILE_A | FILE_C,
    FILE_B | FILE_D,
    FILE_C | FILE_E,
    FILE_D | FILE_F,
    FILE_E | FILE_G,
    FILE_F | FILE_H,
    FILE_G
    
};

// 64bit integers for each square
static const uint64_t SQUARES[65] = {
    
    9223372036854775808U, 4611686018427387904, 2305843009213693952, 1152921504606846976, 576460752303423488, 288230376151711744, 144115188075855872, 72057594037927936,
    36028797018963968, 18014398509481984, 9007199254740992, 4503599627370496, 2251799813685248, 1125899906842624, 562949953421312, 281474976710656,
    140737488355328, 70368744177664, 35184372088832, 17592186044416, 8796093022208, 4398046511104, 2199023255552, 1099511627776,
    549755813888, 274877906944, 137438953472, 68719476736, 34359738368, 17179869184, 8589934592, 4294967296,
    2147483648, 1073741824, 536870912, 268435456, 134217728, 67108864, 33554432, 16777216,
    8388608, 4194304, 2097152, 1048576, 524288, 262144, 131072, 65536,
    32768, 16384, 8192, 4096, 2048, 1024, 512, 256,
    128, 64, 32, 16, 8, 4, 2, 1, 0

};

// Square names
enum {
    
    A8, B8, C8, D8, E8, F8, G8, H8,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A1, B1, C1, D1, E1, F1, G1, H1    
    
};

static const int DIRECTIONS[2][8] = {
    
    { -1, -8, 1, 8, -9, 7, -7, 9 },
    { 1, 8, -1, -8, 9, -7, 7, -9 }    
    
};

static const uint64_t PAWN_STARTRANK[2] = {
    
    RANK_7, RANK_2
    
};

static const uint64_t FrontFileMask[2][64] = {
    
    {
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x8000000000000000, 0x4000000000000000, 0x2000000000000000, 0x1000000000000000, 0x800000000000000, 0x400000000000000, 0x200000000000000, 0x100000000000000,
        0x8080000000000000, 0x4040000000000000, 0x2020000000000000, 0x1010000000000000, 0x808000000000000, 0x404000000000000, 0x202000000000000, 0x101000000000000,
        0x8080800000000000, 0x4040400000000000, 0x2020200000000000, 0x1010100000000000, 0x808080000000000, 0x404040000000000, 0x202020000000000, 0x101010000000000,
        0x8080808000000000, 0x4040404000000000, 0x2020202000000000, 0x1010101000000000, 0x808080800000000, 0x404040400000000, 0x202020200000000, 0x101010100000000,
        0x8080808080000000, 0x4040404040000000, 0x2020202020000000, 0x1010101010000000, 0x808080808000000, 0x404040404000000, 0x202020202000000, 0x101010101000000,
        0x8080808080800000, 0x4040404040400000, 0x2020202020200000, 0x1010101010100000, 0x808080808080000, 0x404040404040000, 0x202020202020000, 0x101010101010000,
        0x8080808080808000, 0x4040404040404000, 0x2020202020202000, 0x1010101010101000, 0x808080808080800, 0x404040404040400, 0x202020202020200, 0x101010101010100
    },
    {
        0x80808080808080, 0x40404040404040, 0x20202020202020, 0x10101010101010, 0x8080808080808, 0x4040404040404, 0x2020202020202, 0x1010101010101,
        0x808080808080, 0x404040404040, 0x202020202020, 0x101010101010, 0x80808080808, 0x40404040404, 0x20202020202, 0x10101010101,
        0x8080808080, 0x4040404040, 0x2020202020, 0x1010101010, 0x808080808, 0x404040404, 0x202020202, 0x101010101,
        0x80808080, 0x40404040, 0x20202020, 0x10101010, 0x8080808, 0x4040404, 0x2020202, 0x1010101,
        0x808080, 0x404040, 0x202020, 0x101010, 0x80808, 0x40404, 0x20202, 0x10101,
        0x8080, 0x4040, 0x2020, 0x1010, 0x808, 0x404, 0x202, 0x101,
        0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    }
    
};

static const uint64_t PassedPawnMask[2][64] = {
    
    {
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0xc000000000000000, 0xe000000000000000, 0x7000000000000000, 0x3800000000000000, 0x1c00000000000000, 0xe00000000000000, 0x700000000000000, 0x300000000000000,
        0xc0c0000000000000, 0xe0e0000000000000, 0x7070000000000000, 0x3838000000000000, 0x1c1c000000000000, 0xe0e000000000000, 0x707000000000000, 0x303000000000000,
        0xc0c0c00000000000, 0xe0e0e00000000000, 0x7070700000000000, 0x3838380000000000, 0x1c1c1c0000000000, 0xe0e0e0000000000, 0x707070000000000, 0x303030000000000,
        0xc0c0c0c000000000, 0xe0e0e0e000000000, 0x7070707000000000, 0x3838383800000000, 0x1c1c1c1c00000000, 0xe0e0e0e00000000, 0x707070700000000, 0x303030300000000,
        0xc0c0c0c0c0000000, 0xe0e0e0e0e0000000, 0x7070707070000000, 0x3838383838000000, 0x1c1c1c1c1c000000, 0xe0e0e0e0e000000, 0x707070707000000, 0x303030303000000,
        0xc0c0c0c0c0c00000, 0xe0e0e0e0e0e00000, 0x7070707070700000, 0x3838383838380000, 0x1c1c1c1c1c1c0000, 0xe0e0e0e0e0e0000, 0x707070707070000, 0x303030303030000,
        0xc0c0c0c0c0c0c000, 0xe0e0e0e0e0e0e000, 0x7070707070707000, 0x3838383838383800, 0x1c1c1c1c1c1c1c00, 0xe0e0e0e0e0e0e00, 0x707070707070700, 0x303030303030300
    },
    {
        0xc0c0c0c0c0c0c0, 0xe0e0e0e0e0e0e0, 0x70707070707070, 0x38383838383838, 0x1c1c1c1c1c1c1c, 0xe0e0e0e0e0e0e, 0x7070707070707, 0x3030303030303,
        0xc0c0c0c0c0c0, 0xe0e0e0e0e0e0, 0x707070707070, 0x383838383838, 0x1c1c1c1c1c1c, 0xe0e0e0e0e0e, 0x70707070707, 0x30303030303,
        0xc0c0c0c0c0, 0xe0e0e0e0e0, 0x7070707070, 0x3838383838, 0x1c1c1c1c1c, 0xe0e0e0e0e, 0x707070707, 0x303030303,
        0xc0c0c0c0, 0xe0e0e0e0, 0x70707070, 0x38383838, 0x1c1c1c1c, 0xe0e0e0e, 0x7070707, 0x3030303,
        0xc0c0c0, 0xe0e0e0, 0x707070, 0x383838, 0x1c1c1c, 0xe0e0e, 0x70707, 0x30303,
        0xc0c0, 0xe0e0, 0x7070, 0x3838, 0x1c1c, 0xe0e, 0x707, 0x303,
        0xc0, 0xe0, 0x70, 0x38, 0x1c, 0xe, 0x7, 0x3,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    }
    
};

static const uint64_t BackwardPawnMask[2][64] = {
    
    {
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x40404040404040, 0xa0a0a0a0a0a0a0, 0x50505050505050, 0x28282828282828, 0x14141414141414, 0xa0a0a0a0a0a0a, 0x5050505050505, 0x2020202020202,
        0x404040404040, 0xa0a0a0a0a0a0, 0x505050505050, 0x282828282828, 0x141414141414, 0xa0a0a0a0a0a, 0x50505050505, 0x20202020202,
        0x4040404040, 0xa0a0a0a0a0, 0x5050505050, 0x2828282828, 0x1414141414, 0xa0a0a0a0a, 0x505050505, 0x202020202,
        0x40404040, 0xa0a0a0a0, 0x50505050, 0x28282828, 0x14141414, 0xa0a0a0a, 0x5050505, 0x2020202,
        0x404040, 0xa0a0a0, 0x505050, 0x282828, 0x141414, 0xa0a0a, 0x50505, 0x20202,
        0x4040, 0xa0a0, 0x5050, 0x2828, 0x1414, 0xa0a, 0x505, 0x202,
        0x40, 0xa0, 0x50, 0x28, 0x14, 0xa, 0x5, 0x2
        
    },
    {
        0x4000000000000000, 0xa000000000000000, 0x5000000000000000, 0x2800000000000000, 0x1400000000000000, 0xa00000000000000, 0x500000000000000, 0x200000000000000,
        0x4040000000000000, 0xa0a0000000000000, 0x5050000000000000, 0x2828000000000000, 0x1414000000000000, 0xa0a000000000000, 0x505000000000000, 0x202000000000000,
        0x4040400000000000, 0xa0a0a00000000000, 0x5050500000000000, 0x2828280000000000, 0x1414140000000000, 0xa0a0a0000000000, 0x505050000000000, 0x202020000000000,
        0x4040404000000000, 0xa0a0a0a000000000, 0x5050505000000000, 0x2828282800000000, 0x1414141400000000, 0xa0a0a0a00000000, 0x505050500000000, 0x202020200000000,
        0x4040404040000000, 0xa0a0a0a0a0000000, 0x5050505050000000, 0x2828282828000000, 0x1414141414000000, 0xa0a0a0a0a000000, 0x505050505000000, 0x202020202000000,
        0x4040404040400000, 0xa0a0a0a0a0a00000, 0x5050505050500000, 0x2828282828280000, 0x1414141414140000, 0xa0a0a0a0a0a0000, 0x505050505050000, 0x202020202020000,
        0x4040404040404000, 0xa0a0a0a0a0a0a000, 0x5050505050505000, 0x2828282828282800, 0x1414141414141400, 0xa0a0a0a0a0a0a00, 0x505050505050500, 0x202020202020200,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    }
    
};

static std::map<unsigned int, unsigned int> castleByKingpos = { {62, WKCASFLAG}, {58, WQCASFLAG}, {6, BKCASFLAG}, {2, BQCASFLAG} };

// Get least significant set bit in unsigned 64bit integer
inline unsigned int lsb_index(const uint64_t bit) {

    return (63 - bitIndex64[((bit & -bit) * debruijn64) >> 58]);
    
}

inline unsigned int msb_index(const uint64_t bit) {
    
    // __builtin_clzll returns undefined behaviour if bit is 0!
    
    return __builtin_clzll(bit);
    
}

// Get least significant set bit in unsigned 64bit integer
inline uint64_t lsb(const uint64_t bit) {
    
    return bit & -bit;
    
}

// Get most significant set bit in unsigned 64bit integer
inline uint64_t msb(const uint64_t bit) {
    
    // __builtin_clzll returns undefined behaviour if bit is 0!
    
    return SQUARES[__builtin_clzll(bit)];
    
}

// Remove least significant set bit in unsigned 64bit integer
inline unsigned int pop_lsb(uint64_t& bit) {
   
   const unsigned int index = lsb_index(bit);
   bit ^= (1ULL << (63 - index));
   return index;
    
}

// Count set bits in unsigned 64bit integer
inline int popcount(const uint64_t bit) {
    
    return __builtin_popcountll(bit);
    
}

// Get rank index (0-7) of given square index
inline unsigned int rank(const unsigned int sq) {

    return (sq >> 3);

}

// Get file index (0-7) of given square index
inline unsigned int file(const unsigned int sq) {

    return (sq & 7);

}

// Get relative rank index for given side of given square index
inline unsigned int relative_rank(const Side side, const unsigned int sq) {
    
    return (side == WHITE) ? 7 - rank(sq) : rank(sq);
    
}

inline unsigned int relative_sq(const Side side, const unsigned int sq) {
    
    return (side == WHITE) ? 63 - sq : sq;
    
}

// Get absolute piecetype without side
inline const PieceType type(const PieceType pt) {
    
    return (pt & 14);
    
}

// Get most forward piece on bitboard for the given side
inline uint64_t most_forward(const Side side, const uint64_t bitboard) {
    
    return (side == WHITE) ? msb(bitboard) : lsb(bitboard);
    
}

// Get most backward piece on file for the given side
inline uint64_t most_backward(const Side side, const uint64_t bitboard) {
    
    return (side == WHITE) ? lsb(bitboard) : msb(bitboard);
    
}

inline unsigned int getKingStartSq(const Side side) {
    
    return (side == WHITE) ? 60 : 4;
    
}

inline unsigned int getRelativeRank(const Side side, const unsigned int rank) {
    
    return (side == WHITE) ? 7 - rank : rank;
    
}

inline PieceType Pawn(const Side side) {
    
    return (PAWN | side);
    
}

inline PieceType Knight(const Side side) {
    
    return (KNIGHT | side);
    
}

inline PieceType Bishop(const Side side) {
    
    return (BISHOP | side);
    
}

inline PieceType Rook(const Side side) {
    
    return (ROOK | side);
    
}

inline PieceType Queen(const Side side) {
    
    return (QUEEN | side);
    
}

inline PieceType King(const Side side) {
    
    return (KING | side);
    
}

inline Score S(const int mg, const int eg) {
    
    Score score;
    
    score.mg = mg;
    score.eg = eg;
    
    return score;
    
}

inline Score operator+=(Score& score, const Score score2) {
    
    score.mg += score2.mg;
    score.eg += score2.eg;
    return score;
    
}

inline Score operator-=(Score& score, const Score score2) {
    
    score.mg -= score2.mg;
    score.eg -= score2.eg;
    return score;
    
}

inline Score operator+(const Score score1, const Score score2) {
    
    Score score;
    score.mg = score1.mg + score2.mg;
    score.eg = score1.eg + score2.eg;
    return score;
    
}

inline Score operator-(const Score score1, const Score score2) {
    
    Score score;
    score.mg = score1.mg - score2.mg;
    score.eg = score1.eg - score2.eg;
    return score;
    
}

inline Score operator*(const Score score1, const int multiply) {
    
    Score score;
    score.mg = score1.mg * multiply;
    score.eg = score1.eg * multiply;
    return score;
    
}

inline unsigned int square(const unsigned int file, const unsigned int rank) {
    
    return file + (rank * 8);
    
}

#endif