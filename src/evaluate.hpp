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

#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.hpp"
#include "bitboards.hpp"
#include "board.hpp"
#include "move.hpp"
#include <complex> // for std::abs

// Values for pieces
static const Value Material[6] = {

    V(  60,  100),
    V( 365,  405),
    V( 390,  430),
    V( 605,  645),
    V(1185, 1260),
    V(   0,    0)

};

// Values for pieces for mvvlva/see
#define PawnValue      100
#define KnightValue    320
#define BishopValue    330
#define RookValue      500
#define QueenValue     950
#define KingValue    99999

// Phases
#define MG 0
#define EG 1

static const int pieceValues[14] = { 0, 0, PawnValue, PawnValue, KnightValue, KnightValue, BishopValue, BishopValue, RookValue, RookValue, QueenValue, QueenValue, KingValue, KingValue };

typedef struct {

    uint64_t mobilityArea[2] = { 0 };
    uint64_t pieceAttacks[2][6] = { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } };
    uint64_t colorAttacks[2] = { 0 };
    uint64_t multiAttacks[2] = { 0 };
    uint64_t blockedPawns[2] = { 0 };
    int kingAttackersWeight[2] = { 0 };
    int kingAttackersNum[2] = { 0 };
    int kingRingAttacks[2] = { 0 };
    unsigned int kingSq[2] = { 0 };
    uint64_t kingRing[2] = { 0 };
    uint64_t weakPawns = 0;
    uint64_t passedPawns = 0;
    uint64_t pawnAttacksSpan[2] = { 0 };
    Value mobility[2] = { V(0, 0), V(0, 0) };

} EvalInfo;

inline int scaled_eval(const int scale, const Value value) {

    return ((value.mg * (256 - scale)) + (value.eg * scale)) / 256;

}

extern Value PieceSquareTable[2][6][64];

extern int KingDistance[64][64];

extern void init_king_distance();
extern void init_psqt();
extern void init_eval();
extern int evaluate(const Board& board);
extern void evaluateInfo(const Board& board);

#endif
