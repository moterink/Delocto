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

#ifndef MOVE_H
#define MOVE_H

#include "types.hpp"

constexpr Move MOVE_NONE = 0;

// Ordered this way for easy detection of promotion types(all promotions have a 1 at first place in binary)
constexpr MoveType NORMAL           = 0x2000;
constexpr MoveType ENPASSANT        = 0x4000;
constexpr MoveType CASTLING         = 0x6000;

constexpr MoveType PROMOTION_QUEEN  = 0x7000;
constexpr MoveType PROMOTION_ROOK   = 0x5000;
constexpr MoveType PROMOTION_BISHOP = 0x3000;
constexpr MoveType PROMOTION_KNIGHT = 0x1000;

// Name strings for each square
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

// Extracts move type from a Move object
inline MoveType move_type(const Move move) {

    return move & 0x7000;

}

// Extracts the origin square from a Move object
inline Square from_sq(const Move move) {

    assert(sq_valid(move & 0x3f));
    assert(move != MOVE_NONE);

    return static_cast<Square>(move & 0x3f);

}

// Extracts the target square from a Move object
inline Square to_sq(const Move move) {

    assert(sq_valid((move & 0xfc0) >> 6));
    assert(move != MOVE_NONE);

    return static_cast<Square>((move & 0xfc0) >> 6);

}

// Constructs a Move object given a origin square, a target square and a move type
inline Move make_move(const Square fromSq, const Square toSq, const MoveType type) {

    return fromSq | (toSq << 6) | type;

}

// Returns the promotion piece given a promotion move type
inline Piecetype prom_piecetype(const MoveType mt) {

    return Piecetype((mt / PROMOTION_KNIGHT + 1) / 2);

}

// Checks wether a given move is a promotion
inline bool is_promotion(const Move move) {

    return move & 0x1000;

}

inline bool is_castling(const Move move) {

    return move_type(move) == CASTLING;

}

// Checks wether a given move is an en-passant capture
inline bool is_ep(const Move move) {

    return move_type(move) == ENPASSANT;

}

extern void print_move(const Move move);
extern void print_bitboard(const Bitboard bitboard);

#endif
