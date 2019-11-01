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

#ifndef MOVE_H
#define MOVE_H

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

    "h1", "g1", "f1", "e1", "d1", "c1", "b1", "a1",
    "h2", "g2", "f2", "e2", "d2", "c2", "b2", "a2",
    "h3", "g3", "f3", "e3", "d3", "c3", "b3", "a3",
    "h4", "g4", "f4", "e4", "d4", "c4", "b4", "a4",
    "h5", "g5", "f5", "e5", "d5", "c5", "b5", "a5",
    "h6", "g6", "f6", "e6", "d6", "c6", "b6", "a6",
    "h7", "g7", "f7", "e7", "d7", "c7", "b7", "a7",
    "h8", "g8", "f8", "e8", "d8", "c8", "b8", "a8"

};

inline MoveType move_type(const Move move) {

    return move & 0x7000;

}

inline unsigned int from_sq(const Move move) {

    assert(sq_valid(move & 0x3f));
    return move & 0x3f;

}

inline unsigned int to_sq(const Move move) {

    assert(sq_valid((move & 0xfc0) >> 6));
    return (move & 0xfc0) >> 6;

}

inline Move make_move(const unsigned int fromsq, const unsigned int tosq, const MoveType type) {

    return fromsq | (tosq << 6) | type;

}

inline Piecetype prom_piecetype(const MoveType mt, const Color color) {

    return (QUEEN + 1 - mt / QUEENPROM) | color;

}

inline bool is_promotion(const Move move) {

    return move & 0x1000;

}

inline bool is_ep(const Move move) {

    return move_type(move) == ENPASSANT;

}

extern void print_move(const Move move);
extern void print_bitboard(const uint64_t bitboard);

#endif
