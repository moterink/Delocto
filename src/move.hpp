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

#ifndef MOVE_H
#define MOVE_H

#include <cstdint>
#include "types.hpp"

#define MOVE_NONE 0

// Ordered this way for easy detection of promotion types(all proms have a 1 at first place in binary)
#define NORMAL     0x2000
#define ENPASSANT  0x4000
#define CASTLING   0x6000
#define QUEENPROM  0x1000
#define ROOKPROM   0x3000
#define BISHOPPROM 0x5000
#define KNIGHTPROM 0x7000

static const std::string SQUARE_NAMES[64] = {

    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"

};

static std::map<unsigned int, PieceType> promPieceTypes = { { QUEENPROM, QUEEN }, {ROOKPROM, ROOK}, { BISHOPPROM, BISHOP }, { KNIGHTPROM, KNIGHT } };

inline const MoveType move_type(const Move move) {

    return (move & 0x7000);

}

inline const unsigned int from_sq(const Move move) {

    return (move & 0x3f);

}

inline const unsigned int to_sq(const Move move) {

    return ((move & 0xfc0) >> 6);

}

inline const Move make_move(const unsigned int fromsq, const unsigned int tosq, const MoveType type) {

    return (fromsq | (tosq << 6) | type);

}

inline PieceType prom_piecetype(const MoveType type, const Side side) {

    return (promPieceTypes[type] | side);

}

inline bool is_promotion(const Move move) {

    return (move & 0x1000);

}

inline bool is_ep(const Move move) {

    return move_type(move) == ENPASSANT;

}

extern void print_move(const Move move);
extern void print_bitboard(const uint64_t bitboard);

#endif
