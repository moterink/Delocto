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

#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.hpp"
#include "bitboards.hpp"
#include "board.hpp"
#include "move.hpp"
#include <complex> // for std::abs

// Scores for pieces
#define PawnValueMg     80
#define PawnValueEg    105
#define KnightValueMg  320
#define KnightValueEg  350
#define BishopValueMg  345
#define BishopValueEg  370
#define RookValueMg    530
#define RookValueEg    570
#define QueenValueMg  1050
#define QueenValueEg  1100
#define KingValueMg      0
#define KingValueEg      0

// Scores for pieces for mvvlva/see
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
    uint64_t attackedSquares[14] = { 0 };
    uint64_t multiAttackedSquares[2] = { 0 };
    uint64_t kingRing[2] = { 0 };
    uint64_t kingZone[2] = { 0 };    
    int kingAttackWeight[2] = { 0 };
    int kingAttackersNum[2] = { 0 };  
    int kingRingAttackWeight[2] = { 0 };
    uint64_t kingZoneAttacks[2] = { 0 };
    int kingRingDefense[2] = { 0 };
    unsigned int kingSq[2] = { 0 };
    uint64_t weakPawns = 0;
    uint64_t passedPawns = 0;
    uint64_t pawnAttacksSpan[2] = { 0 };
    Score mobility[2] = { S(0, 0), S(0, 0) };
    
} EvalInfo;

inline const int scaled_eval(const int scale, const Score score) {

    return ((score.mg * (256 - scale)) + (score.eg * scale)) / 256;
    
}

extern const Score Material[14];
extern Score Pst[14][64];

extern int kingDistance[64][64];

extern void initKingDistance();
extern void initPSQT();
extern void initEval();
extern const int evaluate(const Board& board);
extern void evaluateInfo(const Board& board);

#endif