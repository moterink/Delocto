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

// Maximum Depth and Moves for search
#define MAX_DEPTH 100
#define MAX_MOVES 256

// Scores for mate, draw, infinte, unknown
#define VALUE_MATE       50000
#define VALUE_MATE_MAX  (VALUE_MATE - MAX_DEPTH)
#define VALUE_MATED_MAX (MAX_DEPTH - VALUE_MATE)
#define VALUE_INFINITE        100000
#define VALUE_NONE      100001
#define VALUE_DRAW       0

#define DEPTH_NONE -36

typedef struct {

    int mg = 0;
    int eg = 0;

} Score;

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

static const uint64_t CENTRAL_FILES = FILE_D | FILE_E;

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

static const uint64_t KING_FLANK[8] = {

    FILE_A | FILE_B | FILE_C | FILE_D,
    FILE_A | FILE_B | FILE_C | FILE_D,
    FILE_A | FILE_B | FILE_C | FILE_D,
    FILE_A | FILE_B | FILE_C | FILE_D,
    FILE_E | FILE_F | FILE_G | FILE_H,
    FILE_E | FILE_F | FILE_G | FILE_H,
    FILE_E | FILE_F | FILE_G | FILE_H,
    FILE_E | FILE_F | FILE_G | FILE_H

};

static const uint64_t COLOUR_BASE_SQUARES[2] = {

    RANK_8 | RANK_7 | RANK_6, RANK_1 | RANK_2 | RANK_3

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

static const uint64_t CENTRAL_SQUARES = SQUARES[D4] | SQUARES[D5] | SQUARES[E4] | SQUARES[E5];

static const int DIRECTIONS[2][8] = {

    { -1, -8, 1, 8, -9, 7, -7, 9 },
    { 1, 8, -1, -8, 9, -7, 7, -9 }

};

static const uint64_t PAWN_STARTRANK[2] = {

    RANK_7, RANK_2

};

static const uint64_t PAWN_FIRST_PUSH_RANK[2] = {

    RANK_6, RANK_3

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

inline unsigned int square(const unsigned int file, const unsigned int rank) {

    return file + (rank * 8);

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

inline PieceType pt_index(const PieceType pt) {

    return (type(pt) / 2 - 1);

}

inline uint64_t shift_up(const uint64_t b, const Side side) {

    return (side == WHITE) ? b << 8 : b >> 8;

}

inline uint64_t shift_down(const uint64_t b, const Side side) {

    return (side == WHITE) ? b >> 8 : b << 8;

}

inline uint64_t shift_left(const uint64_t b, const Side side) {

    return (side == WHITE) ? b << 1 : b >> 1;

}

inline uint64_t shift_right(const uint64_t b, const Side side) {

    return (side == WHITE) ? b >> 1 : b << 1;

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

inline Score operator*=(Score& score, const int multiply) {

    score.mg *= multiply;
    score.eg *= multiply;
    return score;

}

inline Score operator/(const Score score1, const int divisor) {

    Score score;
    score.mg = score1.mg / divisor;
    score.eg = score1.eg / divisor;
    return score;

}

inline std::ostream& operator<<(std::ostream& os, const Score& score) {

    return os << "MG: " << score.mg << " | EG: " << score.eg;

}

#endif
