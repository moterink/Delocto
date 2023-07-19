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

#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.hpp"
#include "bitboards.hpp"
#include "board.hpp"
#include "move.hpp"
#include <complex> // for std::abs

// Material values of pieces
static const EvalTerm Material[6] = {

    V(  60,  100),
    V( 365,  405),
    V( 390,  430),
    V( 605,  645),
    V(1185, 1260),
    V(   0,    0)

};

// Game Phases
enum Phase : int {
    PHASE_ENDGAME = 0,
    PHASE_MIDGAME = 128
};

enum ScaleFactor : int {
    SCALE_FACTOR_DRAW = 0,
    SCALE_FACTOR_NORMAL = 128,
    SCALE_FACTOR_MAX = 256
};

struct EvalInfo {

    Bitboard mobilityArea[2] = { 0 };
    Bitboard pieceAttacks[2][6] = { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } };
    Bitboard colorAttacks[2] = { 0 };
    Bitboard multiAttacks[2] = { 0 };
    Bitboard blockedPawns[2] = { 0 };
    int kingAttackersWeight[2] = { 0 };
    int kingAttackersNum[2] = { 0 };
    int kingRingAttacks[2] = { 0 };
    Square kingSq[2] = { 0 };
    Bitboard kingRing[2] = { 0 };
    Bitboard weakPawns = 0;
    Bitboard passedPawns = 0;
    Bitboard pawnAttacksSpan[2] = { 0 };
    EvalTerm mobility[2] = { V(0, 0), V(0, 0) };

};

extern EvalTerm PieceSquareTable[2][6][64];

extern int evaluate(const Board& board, const unsigned threadIndex);
extern void evaluate_info(const Board& board);

namespace Eval {
    extern void init();
}

#endif
