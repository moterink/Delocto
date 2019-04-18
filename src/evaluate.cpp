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

#include "evaluate.hpp"
#include "movegen.hpp"
#include "uci.hpp"

const Score Material[14] = {

    S(0, 0), S(0, 0),
    S(PawnValueMg, PawnValueEg),     S(PawnValueMg, PawnValueEg),
    S(KnightValueMg, KnightValueEg), S(KnightValueMg, KnightValueEg),
    S(BishopValueMg, BishopValueEg), S(BishopValueMg, BishopValueEg),
    S(RookValueMg, RookValueEg),     S(RookValueMg, RookValueEg),
    S(QueenValueMg, QueenValueEg),   S(QueenValueMg, QueenValueEg),
    S(KingValueMg, KingValueEg),     S(KingValueMg, KingValueEg)

};

Score Pst[14][64];

static const Score PstValues[6][32] = {

    // Pawns
    {

        S( 0,  0), S( 0,  0), S( 0,  0), S( 0,  0),
        S(-2,  4), S(12, -6), S(-5,  1), S(-1, 16),
        S(-3,  7), S(-4, -3), S(-3,  2), S( 0,  1),
        S(-3,  6), S( 5,  7), S( 3,  5), S( 9, -3),
        S(-7,  0), S( 0,  2), S( 9, -5), S(15, -4),
        S(-8, -3), S(-1, -1), S( 8,  3), S( 8,  2),
        S(-6,  2), S( 3, -2), S( 3,  4), S( 2, -1),
        S( 0,  0), S( 0,  0), S( 0,  0), S( 0,  0)

    },
    // Knights
    {

        S(-135, -40), S(-25, -30), S(-15, -20), S(-10, -15),
        S( -20, -30), S(-10, -20), S(  0, -10), S(  5,  -5),
        S(  -5, -20), S(  5, -12), S( 15,   0), S( 26,   5),
        S(  -5, -15), S(  5,  -4), S( 15,   4), S( 21,  15),
        S( -14, -18), S(  0,  -4), S( 13,   5), S( 19,  12),
        S( -30, -21), S( -5,  -7), S(  2,  -5), S(  3,   6),
        S( -35, -30), S(-20, -24), S( -8,  -6), S( -4,   4),
        S( -50, -40), S(-40, -30), S(-30, -20), S(-24, -12)

    },
    // Bishops
    {

        S(-18, -24), S(-5, -13), S( -8,  -9), S(-12, -6),
        S( -8, -12), S( 0,  -6), S( -2,  -3), S( -2,  0),
        S( -6,  -9), S(-2,  -3), S(  4,   0), S(  1,  3),
        S( -4,  -6), S(10,   1), S(  6,   3), S(  4,  6),
        S( -4,  -6), S(11,   0), S(  9,  -2), S(  4,  6),
        S( -6,  -9), S(11,   0), S(  9,  -1), S(  4,  5),
        S( -9, -12), S( 8,  -4), S(  5,  -2), S( -3, -1),
        S(-18, -24), S(-5, -12), S(-11, -14), S(-14, -9)

    },
    // Rooks
    {

        S( -9, 0), S(-6, 0), S(-5, 0), S(-2, 0),
        S( -5, 0), S( 1, 0), S( 3, 0), S( 5, 0),
        S( -8, 0), S(-2, 0), S( 0, 0), S( 1, 0),
        S( -8, 0), S(-2, 0), S( 0, 0), S( 0, 0),
        S( -8, 0), S(-2, 0), S( 0, 0), S( 1, 0),
        S( -8, 0), S(-3, 0), S(-1, 0), S( 1, 0),
        S( -8, 0), S(-3, 0), S(-1, 0), S( 0, 0),
        S(-10, 0), S(-6, 0), S(-5, 0), S(-4, 0)

    },
    // Queens
    {

        S( 0, -24), S( 0, -16), S( 0, -12), S( 0, -12),
        S( 0, -16), S( 3, -10), S( 2,  -4), S( 2,  -2),
        S(-1, -12), S( 2,  -6), S( 3,  -2), S( 5,   1),
        S(-2,  -8), S( 4,  -2), S( 3,   4), S( 2,   8),
        S(-1,  -8), S( 3,  -2), S( 4,   4), S( 2,   7),
        S( 0, -10), S( 2,  -4), S( 3,  -3), S( 4,   2),
        S(-1, -16), S( 2,  -8), S( 4,  -6), S( 3,  -1),
        S(-4, -26), S(-3, -12), S(-2, -14), S(-1, -12)

    },
    // King
    {

        S( 26,  3), S( 36, 22), S( 19, 33), S(  0, 37),
        S( 36, 20), S( 53, 41), S( 26, 50), S(  8, 57),
        S( 49, 39), S( 66, 64), S( 35, 73), S( 17, 72),
        S( 62, 45), S( 73, 67), S( 47, 83), S( 27, 84),
        S( 73, 44), S( 77, 70), S( 61, 70), S( 45, 74),
        S( 83, 34), S(102, 57), S( 73, 69), S( 45, 68),
        S(110, 17), S(123, 36), S( 99, 59), S( 75, 55),
        S(111,  0), S(135, 18), S(112, 31), S( 81, 35)

    }

};

static const int Imbalance[2][5][5] = {

    {
        {  0 },
        {  3,  0 },
        {  2, -7,  0 },
        { -3, -5, -6,  0 },
        {  1, -4,  2, -11, 0 }
    },
    {
        {  0 },
        {  5,  0 },
        {  2, -4,  0 },
        {  2, -7, -9, 0 },
        {  6, -8,  6, -8, 0 }
    }

};

static int Outpost[2][2][64];

static const uint64_t OutpostSquares[2][2] = { {0x7effff000000, 0x3c7e7e660000}, {0xffff7e0000, 0x667e7e3c0000} };

static const int OutpostValues[2][64] = {

    {

         0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,
         0,  2,  5,  8,  8,  5,  2,  0,
         8, 10, 18, 20, 20, 18, 10,  8,
        12, 16, 18, 20, 20, 18, 16, 12,
         0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0

    },
    {

        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 2, 4, 4, 2, 0, 0,
        0, 1, 4, 9, 9, 4, 1, 0,
        0, 1, 4, 9, 9, 4, 1, 0,
        0, 2, 1, 0, 0, 1, 2, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0

    }

};

static const uint64_t KingRing[2][64] = {

    {
        0x40c0000000000000, 0xa0e0000000000000, 0x5070000000000000, 0x2838000000000000, 0x141c000000000000, 0xa0e000000000000, 0x507000000000000, 0x203000000000000,
        0xc040c00000000000, 0xe0a0e00000000000, 0x7050700000000000, 0x3828380000000000, 0x1c141c0000000000, 0xe0a0e0000000000, 0x705070000000000, 0x302030000000000,
        0xc040c000000000, 0xe0a0e000000000, 0x70507000000000, 0x38283800000000, 0x1c141c00000000, 0xe0a0e00000000, 0x7050700000000, 0x3020300000000,
        0xc040c0000000, 0xe0a0e0000000, 0x705070000000, 0x382838000000, 0x1c141c000000, 0xe0a0e000000, 0x70507000000, 0x30203000000,
        0xc040c00000, 0xe0a0e00000, 0x7050700000, 0x3828380000, 0x1c141c0000, 0xe0a0e0000, 0x705070000, 0x302030000,
        0xc040c000, 0xe0a0e000, 0x70507000, 0x38283800, 0x1c141c00, 0xe0a0e00, 0x7050700, 0x3020300,
        0xc040c0, 0xe0a0e0, 0x705070, 0x382838, 0x1c141c, 0xe0a0e, 0x70507, 0x30203,
        0xc040, 0xe0a0, 0x7050, 0x3828, 0x1c14, 0xe0a, 0x705, 0x302
    },
    {
        0x40c0000000000000, 0xa0e0000000000000, 0x5070000000000000, 0x2838000000000000, 0x141c000000000000, 0xa0e000000000000, 0x507000000000000, 0x203000000000000,
        0xc040c00000000000, 0xe0a0e00000000000, 0x7050700000000000, 0x3828380000000000, 0x1c141c0000000000, 0xe0a0e0000000000, 0x705070000000000, 0x302030000000000,
        0xc040c000000000, 0xe0a0e000000000, 0x70507000000000, 0x38283800000000, 0x1c141c00000000, 0xe0a0e00000000, 0x7050700000000, 0x3020300000000,
        0xc040c0000000, 0xe0a0e0000000, 0x705070000000, 0x382838000000, 0x1c141c000000, 0xe0a0e000000, 0x70507000000, 0x30203000000,
        0xc040c00000, 0xe0a0e00000, 0x7050700000, 0x3828380000, 0x1c141c0000, 0xe0a0e0000, 0x705070000, 0x302030000,
        0xc040c000, 0xe0a0e000, 0x70507000, 0x38283800, 0x1c141c00, 0xe0a0e00, 0x7050700, 0x3020300,
        0xc040c0, 0xe0a0e0, 0x705070, 0x382838, 0x1c141c, 0xe0a0e, 0x70507, 0x30203,
        0xc040, 0xe0a0, 0x7050, 0x3828, 0x1c14, 0xe0a, 0x705, 0x302
    }

};

static const uint64_t KingZone[2][64] = {

    {
        0x40c0000000000008, 0xa0e0000000000008, 0x5070000000000008, 0x2838000000000008, 0x141c000000000008, 0xa0e000000000008, 0x507000000000008, 0x203000000000008,
        0xc040c00000000008, 0xe0a0e00000000008, 0x7050700000000008, 0x3828380000000008, 0x1c141c0000000008, 0xe0a0e0000000008, 0x705070000000008, 0x302030000000008,
        0xc0c040c000000000, 0xe0e0a0e000000000, 0x7070507000000000, 0x3838283800000000, 0x1c1c141c00000000, 0xe0e0a0e00000000, 0x707050700000000, 0x303020300000000,
        0xc0c040c0000000, 0xe0e0a0e0000000, 0x70705070000000, 0x38382838000000, 0x1c1c141c000000, 0xe0e0a0e000000, 0x7070507000000, 0x3030203000000,
        0xc0c040c00000, 0xe0e0a0e00000, 0x707050700000, 0x383828380000, 0x1c1c141c0000, 0xe0e0a0e0000, 0x70705070000, 0x30302030000,
        0xc0c040c000, 0xe0e0a0e000, 0x7070507000, 0x3838283800, 0x1c1c141c00, 0xe0e0a0e00, 0x707050700, 0x303020300,
        0xc0c040c0, 0xe0e0a0e0, 0x70705070, 0x38382838, 0x1c1c141c, 0xe0e0a0e, 0x7070507, 0x3030203,
        0xc0c040, 0xe0e0a0, 0x707050, 0x383828, 0x1c1c14, 0xe0e0a, 0x70705, 0x30302

    },
    {   0x40c0c00000000000, 0xa0e0e00000000000, 0x5070700000000000, 0x2838380000000000, 0x141c1c0000000000, 0xa0e0e0000000000, 0x507070000000000, 0x203030000000000,
        0xc040c0c000000000, 0xe0a0e0e000000000, 0x7050707000000000, 0x3828383800000000, 0x1c141c1c00000000, 0xe0a0e0e00000000, 0x705070700000000, 0x302030300000000,
        0xc040c0c0000000, 0xe0a0e0e0000000, 0x70507070000000, 0x38283838000000, 0x1c141c1c000000, 0xe0a0e0e000000, 0x7050707000000, 0x3020303000000,
        0xc040c0c00000, 0xe0a0e0e00000, 0x705070700000, 0x382838380000, 0x1c141c1c0000, 0xe0a0e0e0000, 0x70507070000, 0x30203030000,
        0xc040c0c000, 0xe0a0e0e000, 0x7050707000, 0x3828383800, 0x1c141c1c00, 0xe0a0e0e00, 0x705070700, 0x302030300,
        0xc040c0c0, 0xe0a0e0e0, 0x70507070, 0x38283838, 0x1c141c1c, 0xe0a0e0e, 0x7050707, 0x3020303,
        0xc040c3, 0xe0a0e3, 0x705073, 0x38283b, 0x1c141f, 0xe0a0f, 0x70507, 0x30203,
        0xc043, 0xe0a3, 0x7053, 0x382b, 0x1c17, 0xe0b, 0x707, 0x303
    }

};

static const Score Mobility[4][28] = {

    // Knights
    { S(-29, -30), S(-9, -6), S(-3, -2), S(-1, 2), S(3, 5), S(6, 8), S(8, 11), S(12, 14), S(14, 15) },
    // Bishops
    { S(-21, -32), S(-6, -7), S(6, 3), S(12, 7), S(15, 11), S(21, 16), S(23, 19), S(26, 23), S(29, 30), S(32, 33), S(32, 35), S(33, 37), S(33, 40), S(33, 42) },
    // Rooks
    { S(-23, -36), S(-7, -8), S(-5, 8), S(-4, 19), S(-3, 25), S(-1, 36), S(4, 42), S(8, 47), S(12, 53), S(12, 57), S(14, 63), S(15, 65), S(17, 67), S(20, 68), S(23, 68) },
    // Queens
    { S(-25, -40), S(-8, -2), S(0, 6), S(1, 9), S(5, 18), S(8, 23), S(12, 28), S(15, 32), S(16, 34), S(19, 38), S(21, 42), S(24, 45), S(25, 46), S(27, 46), S(27, 49), S(28, 51), S(29, 52), S(30, 54), S(32, 56), S(34, 58), S(35, 62), S(39, 66), S(40, 70), S(40, 75), S(41, 79), S(43, 81), S(44, 83), S(45, 84) }

};

// Pawns
static const Score pawnDoubledPenalty           = S(8, 14);
static const Score pawnIsolatedPenalty[2]       = { S(11, 12), S(5, 7) };
static const Score pawnBackwardPenalty[2]       = { S(17, 11), S(10, 5) };
static const Score pawnLeverBonus[8]            = { S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(7, 6), S(13, 13), S(0, 0), S(0, 0) };
static const Score pawnConnectedBonus[2][8]  = {

    { S(0, 0), S(5, 0), S(10, 0), S(8, 4), S(32, 16), S(42, 31), S(73, 73), S(0, 0) },
    { S(0, 0), S(3, 0), S( 5, 0), S(4, 2), S(16,  8), S(21, 16), S(36, 36), S(0, 0) }

};
static const Score pawnPhalanxBonus[2][8]  = {

    { S(0, 0), S(8, 0), S(9, 0), S(16, 2), S(37, 18), S(57, 43), S(105, 105), S(0, 0) },
    { S(0, 0), S(4, 0), S(4, 0), S( 9, 1), S(18,  9), S(29, 21), S( 52,  52), S(0, 0) }

};
static const Score pawnSupportBonus             = S(6, 2);
static const Score pawnPassedBonus[8]           = { S(0, 0), S(2, 4), S(4, 6), S(12, 16), S(32, 32), S(69, 78), S(95, 110), S(0, 0) };//{ S(0, 0), S(-9, 25), S(4, 13), S(13, 22), S(41, 52), S(84, 105), S(182, 196), S(0, 0) }; //{ S(0, 0), S(3, 4), S(3, 8), S(20, 22), S(40, 42), S(85, 90), S(140, 150), S(0, 0) };
static const Score passerFileBonus[8]           = { S(3, 4), S(2, 3), S(-2, -8), S(-9, -5), S(-9, -5), S(-2, -8), S(2, 3), S(3, 4) };
static const Score pawnCandidateBonus[8]        = { S(0, 0), S(0, 0), S(5, 15),  S(10, 30), S(20, 45), S(30, 50),   S(40, 60),   S(0, 0) };

static const Score passerPushBonus              = S(1, 3);
static const Score passerSafePushBonus          = S(3, 4);
static const Score passerFullyDefendedBonus     = S(6, 8);
static const Score passerNoAttackersBonus       = S(3, 4);
static const Score passerAttackedPenalty        = S(3, 6);

// Knights
static const Score knightPairPenalty            = S(13, 14);

// Bishops
static const Score bishopPairBonus              = S(32, 37);
static const Score bishopPawnsSameColorPenalty  = S( 4,  5);

// Minors
static const unsigned int outpostProtectorBonus[2] = { 6, 3 };
static const Score OutpostReachable[2]             = { S(8, 0), S(4, 0) };
static const Score minorPawnShield                 = S(4, 1);

// Rooks
static const Score rookOpenFileBonus            = S(19,  9);
static const Score rookSemiOpenFileBonus        = S( 6,  4);
static const Score rookPawnAlignBonus           = S( 3, 12);
static const unsigned int rookTrappedPenalty    = 40;
static const Score rookPairPenalty              = S(20, 16);

// King
static const unsigned int undefendedRingPenalty      =  18;
static const unsigned int attackedRingPenalty[4]     = { 12, 11, 14, 8 };
static const unsigned int noQueenWeight              =  80;
static const unsigned int queenSafeCheckWeight       =  90;
static const unsigned int rookSafeCheckWeight        = 120;
static const unsigned int bishopSafeCheckWeight      =  65;
static const unsigned int knightSafeCheckWeight      = 100;
static const unsigned int attackerWeight[4]          = { 25, 36, 41, 21 };
static const unsigned int defenderBonus[4]           = { 28, 26, 25, 27 };
static const unsigned int defenderDistancePenalty[4] = {  6,  7,  8,  8 };
static const unsigned int closeEnemyPenalty          = 3;
static const unsigned int castlingRightBonus[3]      = { 0, 12, 18 };
static const unsigned int kingOpenFilePenalty[2]     = {  6, 8 };
static const unsigned int kingSemiOpenFilePenalty[2] = {  2, 6 };

static const int kingPawnShelterValues[2][4][8] = {

    {
        { -31,  0,  -6, -16, -29, -31, -32, 0 },
        { -34,  0, -13, -31, -32, -35, -38, 0 },
        { -39,  0, -27, -36, -25, -37, -39, 0 },
        { -28, -2, -16, -25, -29, -30, -31, 0 }
    },
    {
        { -34,  0,  -2,  -9, -28, -32, -33, 0 },
        { -38,  0, -14, -35, -37, -38, -40, 0 },
        { -41,  0, -24, -38, -27, -40, -42, 0 },
        { -24,  0, -14, -23, -31, -34, -37, 0 }
    }

};

static const int kingPawnStormValues[3][4][8] = {
    // Blocked by pawn
    {
        { 0, 0, 0, 0,  8, 10, 0, 0 },
        { 0, 0, 0, 1, 11, 37, 0, 0 },
        { 0, 0, 0, 0,  7, 42, 0, 0 },
        { 0, 0, 0, 6,  9, 46, 0, 0 }
    },
    // Blocked by king
    {
        { 0, 0, 0, 16, 24, -87, -92, 0 },
        { 0, 0, 0,  5, 16,  56,  25, 0 },
        { 0, 0, 0, 11, 17,  53,  28, 0 },
        { 0, 0, 0,  6, 22,  48,  23, 0 }
    },
    // Unopposed
    {
        { 0, 0, 0, 13, 19, 54, 30, 0 },
        { 0, 0, 0,  5, 11, 61, 27, 0 },
        { 0, 0, 0, 10, 17, 45, 21, 0 },
        { 0, 0, 0, 12, 20, 53, 30, 0 }
    }

};

// Threats
static const Score safePawnAttack         = S(70, 74);
static const Score loosePawnAttack        = S(15,  6);
static const Score loosePawnWeight        = S(16, 28);
static const Score loosePieceWeight       = S(22, 13);
static const Score pawnPushThreat         = S(16,  9);
static const Score weakPawn               = S( 2, 10);
static const Score pieceVulnerable        = S( 6,  0);
static const Score minorAttackWeight[5] = {

    S(0, 0), S(16, 18), S(23, 18), S(28, 46), S(20, 50)

};
static const Score rookAttackWeight[5] = {

    S(0, 10), S(15, 29), S(15, 24), S(0, 16), S(15, 17)
};

// Tempo Bonus
static const int tempoBonus = 12;

int kingDistance[64][64];
static int kingPawnShelter[2][8][8];
static int kingPawnStorm[3][8][8];

void initKingDistance() {

    unsigned int findex, tindex;

    for (findex = 0; findex < 64; findex++) {
        for (tindex = 0; tindex < 64; tindex++) {
            kingDistance[findex][tindex] = std::max(std::abs((int)rank(tindex) - (int)rank(findex)), std::abs((int)file(tindex) - (int)file(findex)));
        }
    }

}

void initPSQT() {

    for (unsigned int pt = PAWN; pt <= KING; pt+=2) {
        for (unsigned int sq = 0; sq < 32;  sq++) {
            const unsigned int r = sq / 4;
            const unsigned int f = sq & 0x3;
            Score score = PstValues[(pt - 2) / 2][sq];

            Pst[(pt | WHITE)][8 * r + f] = score;
            Pst[(pt | WHITE)][8 * r + (7 - f)] = score;
            Pst[(pt | BLACK)][8 * (7 - r) + f] = score;
            Pst[(pt | BLACK)][8 * (7 - r) + (7 - f)] = score;

        }
    }

}

void initEval() {

    // Mirror Outpost Values
    for (unsigned int sq = 0; sq < 64; sq++) {
        Outpost[0][0][sq] = OutpostValues[0][sq];
        Outpost[0][1][sq] = OutpostValues[1][sq];
        Outpost[1][0][63 - sq] = OutpostValues[0][sq];
        Outpost[1][1][63 - sq] = OutpostValues[1][sq];
    }

    // Mirror King Pawn Shelter Values
    for (unsigned int t = 0; t < 2; t++) {
        for (unsigned int f = 0; f < 4; f++) {
            for (unsigned int r = 0; r < 8; r++) {
                kingPawnShelter[t][f][r]     = kingPawnShelterValues[t][f][r];
                kingPawnShelter[t][7 - f][r] = kingPawnShelterValues[t][f][r];
            }
        }
    }

    // Mirror King Pawn Storm Values
    for (unsigned int t = 0; t < 3; t++) {
        for (unsigned int f = 0; f < 4; f++) {
            for (unsigned int r = 0; r < 8; r++) {
                kingPawnStorm[t][f][r]       = kingPawnStormValues[t][f][r];
                kingPawnStorm[t][7 - f][r]   = kingPawnStormValues[t][f][r];
            }
        }
    }

}

bool Board::checkMaterialDraw(const unsigned int pieceCount) const {

    if (bitboards[WHITE_PAWN] || bitboards[BLACK_PAWN] || bitboards[WHITE_ROOK] || bitboards[BLACK_ROOK] || bitboards[WHITE_QUEEN] || bitboards[BLACK_QUEEN]) {
        return false;
    }

    switch(pieceCount) {

        case 2: return true;
        case 3: return true;
        case 4:
            {
                if (piececounts[WHITE_BISHOP] >= 2 || piececounts[BLACK_BISHOP] >= 2 || (bitboards[WHITE_KNIGHT] && bitboards[WHITE_BISHOP]) || (bitboards[BLACK_KNIGHT] && bitboards[BLACK_BISHOP]) || piececounts[WHITE_KNIGHT] == 2 || piececounts[BLACK_KNIGHT] == 2) {
                    return false;
                }
                return true;
            }
        default: assert(false);
    }

    assert(false);
    return false;

}

static void update_attack_info(Side side, PieceType pt, unsigned int sq, uint64_t moves, EvalInfo& info) {

    unsigned int pindex = pt_index(pt) - 1;

    info.attackedSquares[pt] |= moves;
    info.multiAttackedSquares[side] |= info.attackedSquares[side] & moves;
    info.attackedSquares[side] |= moves;

    const uint64_t kingAttacks = moves & info.kingZone[!side];

    if (kingAttacks) {

        info.kingAttackWeight[!side] += attackerWeight[pindex];
        info.kingAttackersNum[!side]++;
        info.kingRingAttackWeight[!side] += popcount(kingAttacks & info.kingRing[!side]) * attackedRingPenalty[pindex];

    }

    info.kingRingDefense[side] += defenderBonus[pindex] - defenderDistancePenalty[pindex] * kingDistance[sq][info.kingSq[side]];

}

static const Score evaluate_knights(const Board& board, const Side side, EvalInfo& info) {

    Score score;

    uint64_t knights = board.pieces(KNIGHT, side);
    while (knights) {

        unsigned int sq = pop_lsb(knights);
        uint64_t moves = generateKnightMoves(sq, 0);

        const uint64_t outpostProtectors = attackingPawns[side][sq] & board.pieces(PAWN, side);

        if (Outpost[side][0][sq] != 0 && outpostProtectors && !(SQUARES[sq] & info.pawnAttacksSpan[!side])) {
            score.mg += Outpost[side][0][sq] + (popcount(outpostProtectors) * outpostProtectorBonus[0]);
        } else if (moves & OutpostSquares[side][0]) {
            score += OutpostReachable[0];
        }

        if ((relative_rank(side, sq) <= 3) && (SQUARES[sq + DIRECTIONS[side][UP]] & board.pieces(PAWN, side))) {
            score += minorPawnShield;
        }

        info.mobility[side] += Mobility[0][popcount(moves & info.mobilityArea[side])];
        update_attack_info(side, Knight(side), sq, moves, info);

    }

    return score;

}

static const Score evaluate_bishops(const Board& board, const Side side, EvalInfo& info) {

    Score score;

    uint64_t bishops = board.pieces(BISHOP, side);
    while (bishops) {

        unsigned int sq = pop_lsb(bishops);

        // Exclude queen for xrays
        uint64_t moves = generateBishopMoves(sq, board.pieces(ALLPIECES) & ~board.pieces(QUEEN, side), 0);

        const uint64_t outpostProtectors = attackingPawns[side][sq] & board.pieces(PAWN, side);

        if (Outpost[side][1][sq] != 0 && outpostProtectors && !(SQUARES[sq] & info.pawnAttacksSpan[!side])) {
            score.mg += Outpost[side][1][sq] + (popcount(outpostProtectors) * outpostProtectorBonus[1]);
        } else if ((moves & ~board.pieces(side)) & OutpostSquares[side][1]) {
            score += OutpostReachable[1];
        }

        if ((relative_rank(side, sq) <= 3) && (SQUARES[sq + DIRECTIONS[side][UP]] & board.pieces(PAWN, side))) {
            score += minorPawnShield;
        }

        score -= (SQUARES[sq] & WHITE_SQUARES) ?  bishopPawnsSameColorPenalty * popcount(WHITE_SQUARES & board.pieces(PAWN, side)) : bishopPawnsSameColorPenalty * popcount(BLACK_SQUARES & board.pieces(PAWN, side));

        info.mobility[side] += Mobility[1][popcount(moves & info.mobilityArea[side])];
        update_attack_info(side, Bishop(side), sq, moves, info);

    }

    return score;

}

static const Score evaluate_rooks(const Board& board, const Side side, EvalInfo& info) {

    Score score;

    uint64_t rooks = board.pieces(ROOK, side);
    while (rooks) {

        unsigned int sq = pop_lsb(rooks);

        // Exclude queens and rooks for xrays
        uint64_t moves = generateRookMoves(sq, board.pieces(ALLPIECES) & ~(board.pieces(QUEEN, side) | board.pieces(ROOK, side)), 0);

        const unsigned int f = file(sq);

        if (!(FILES[f] & (board.pieces(WHITE_PAWN) | board.pieces(BLACK_PAWN)))) {
            score += rookOpenFileBonus;
        } else if (!(FILES[f] & board.pieces(PAWN, side))) {
            score += rookSemiOpenFileBonus;
        } else {

            const unsigned int ksq = relative_sq(side, info.kingSq[side]);
            const unsigned int rsq = relative_sq(side, sq);

            if (((rsq == A8 || rsq == A7 || rsq == B8) && (ksq == B8 || ksq == C8)) || ((rsq == H8 || rsq == H7 || rsq == G8) && (ksq == F8 || ksq == G8))) {
                score.mg -= rookTrappedPenalty;
            }

        }

        // Bonus for aligning on file with enemy pawn
        if (relative_rank(side, sq) >= 4) {
            score += rookPawnAlignBonus * popcount(moves & board.pieces(PAWN, !side));
        }

        info.mobility[side] += Mobility[2][popcount(moves & info.mobilityArea[side])];
        update_attack_info(side, Rook(side), sq, moves, info);

    }

    return score;

}

static const Score evaluate_queens(const Board& board, const Side side, EvalInfo& info) {

    Score score;

    uint64_t queens = board.pieces(QUEEN, side);
    while (queens) {

        unsigned int sq = pop_lsb(queens);
        uint64_t moves = generateQueenMoves(sq, board.pieces(ALLPIECES), 0);

        info.mobility[side] += Mobility[3][popcount(moves & info.mobilityArea[side])];
        update_attack_info(side, Queen(side), sq, moves, info);

    }

    return score;

}

static const Score evaluate_pawns(const Board& board, const Side side, EvalInfo& info) {

    Score score;

    const uint64_t ownPawns = board.pieces(PAWN, side);
    const uint64_t oppPawns = board.pieces(PAWN, !side);
    const uint64_t allPawns = ownPawns | oppPawns;

    uint64_t pawns = ownPawns;

    while (pawns) {

        const unsigned int sq = pop_lsb(pawns);
        const unsigned int f  = file(sq);
        const unsigned int r  = relative_rank(side, sq);

        const uint64_t front      = FrontFileMask[side][sq];
        const uint64_t neighbours = ADJ_FILES[f] & ownPawns;
        const uint64_t stoppers   = PassedPawnMask[side][sq] & oppPawns;

        info.pawnAttacksSpan[side] |= PawnAttacksSpan[side][sq];

        const bool doubled       = front & ownPawns;
        const bool opposed       = front & oppPawns;
        const bool lever         = AttackBitboards[Pawn(side)][sq] & oppPawns;
        const bool isolated      = !neighbours;
        const bool passed        = !stoppers;
        const uint64_t connected = (neighbours & RANKS[rank(sq + DIRECTIONS[side][DOWN])]);
        const uint64_t phalanx   = (neighbours & RANKS[rank(sq)]);

        bool backward = false;

        if (!isolated && !phalanx && r <= 4 && !lever) {
            const uint64_t br = RANKS[rank(lsb_index(most_backward(side, neighbours | stoppers)))];
            backward = (br | ((side == WHITE) ? (ADJ_FILES[f] & br) << 8 : (ADJ_FILES[f] & br) >> 8)) & stoppers;
        }

        if (doubled) {
            score -= pawnDoubledPenalty;
        } else if (passed) {
            info.passedPawns |= SQUARES[sq];
        }

        if (connected) {
            score += pawnConnectedBonus[opposed][r] + (pawnSupportBonus * popcount(connected));
        }

        if (phalanx) {
            score += pawnPhalanxBonus[opposed][r];
        } else if (isolated) {
            score -= pawnIsolatedPenalty[opposed];

        } else if (backward) {
            score -= pawnBackwardPenalty[opposed];
        }

        if (lever) {
            score += pawnLeverBonus[r];
        }

        if (!opposed && (backward || isolated)) {
            info.weakPawns |= SQUARES[sq];
        }

    }

    return score;

}

static const Score evaluate_king_safety(const Board& board, const Side side, const EvalInfo& info) {

    Score score = S(0, 0);
    int pawnScore = 70;

    const uint64_t ownPawns = board.pieces(PAWN, side);
    const uint64_t oppPawns = board.pieces(PAWN, !side);

    const int kingFile             = file(info.kingSq[side]);
    const int centralFile          = std::max(1, std::min(kingFile, 6));
    const uint64_t kingSpan        = KingShelterSpan[side][info.kingSq[side] - (kingFile - centralFile)];
    const unsigned int kingRelRank = relative_rank(side, info.kingSq[side]);

    for (unsigned int f = centralFile - 1; f <= centralFile + 1; f++) {

        const uint64_t owns = FILES[f] & kingSpan & ownPawns;
        const uint64_t opps = FILES[f] & kingSpan & oppPawns;

        if (owns) {

            pawnScore += kingPawnShelter[f == kingFile][f][relative_rank(side, lsb_index(most_backward(side, owns)))];

        } else {

            pawnScore += kingPawnShelter[f == kingFile][f][0];

            if (!opps) {
                score.mg -= kingOpenFilePenalty[f == kingFile];
            }

        }

        if (opps) {

            const unsigned int sq = lsb_index(most_forward(!side, opps));
            pawnScore -= kingPawnStorm[owns ? 0 : (info.kingSq[side] == sq + DIRECTIONS[!side][UP]) ? 1 : 2][f][relative_rank(!side, sq)];

            if (!owns) {
                score.mg -= kingSemiOpenFilePenalty[f == kingFile];
            }

        }

    }

    // Don't calculate king safety if only one attacker and no queen on board
    if (info.kingAttackersNum[side] > (1 - popcount(board.pieces(QUEEN, !side)))) {

        const uint64_t zone = KingZone[side][info.kingSq[side]];
        const uint64_t ring = KingRing[side][info.kingSq[side]];

        const uint64_t undefendedRing     = (ring & info.attackedSquares[!side]) & ~info.multiAttackedSquares[side];

        const uint64_t knightCheckSquares = generateKnightMoves(info.kingSq[side], board.pieces(side));
        const uint64_t bishopCheckSquares = generateBishopMoves(info.kingSq[side], board.pieces(ALLPIECES), 0);
        const uint64_t rookCheckSquares   = generateRookMoves(info.kingSq[side], board.pieces(ALLPIECES), 0);
        uint64_t safeSquares              = ~board.pieces(!side) & (~info.attackedSquares[side] | ((info.attackedSquares[!side] & ~info.multiAttackedSquares[side] & (info.attackedSquares[Queen(side)] | ring | ~info.attackedSquares[side])) & info.multiAttackedSquares[!side]));

        int safety = 0;

        safety -= (info.kingAttackersNum[side] * info.kingAttackWeight[side]) / 6;

        safety -= popcount(undefendedRing) * undefendedRingPenalty;

        safety -= info.kingRingAttackWeight[side];

        if ((info.attackedSquares[Queen(!side)] & (bishopCheckSquares | rookCheckSquares)) & safeSquares) {
            safety -= queenSafeCheckWeight;
        }

        if ((info.attackedSquares[Rook(!side)] & rookCheckSquares) & safeSquares) {
            safety -= rookSafeCheckWeight;
        }

        if ((info.attackedSquares[Bishop(!side)] & bishopCheckSquares) & safeSquares) {
            safety -= bishopSafeCheckWeight;
        }

        if ((info.attackedSquares[Knight(!side)] & knightCheckSquares) & safeSquares) {
            safety -= knightSafeCheckWeight;
        }

        safety -= popcount(zone & board.pieces(!side)) * closeEnemyPenalty;

        safety += info.kingRingDefense[side] / 3;

        safety += pawnScore / 5;

        if (!board.pieces(QUEEN, !side)) {
            safety += noQueenWeight;
        }

        score.mg += std::min(0, safety);

    }

    score.mg += pawnScore;

    score.mg += castlingRightBonus[popcount(board.castleRights() & CASTLE_MASKS[side])];

    score.mg += info.kingRingDefense[side] / 2;

    return S(std::min(80, score.mg), score.eg);

}

static const Score evaluate_passers(const Board& board, const Side side, const EvalInfo& info) {

    Score score;

    uint64_t passers = info.passedPawns & board.pieces(side);

    while (passers) {

        const unsigned int sq      = pop_lsb(passers);
        const unsigned int blocksq = sq + DIRECTIONS[side][UP];
        const int r                = relative_rank(side, sq);
        const unsigned int f       = file(sq);
        const unsigned int rfactor = (r - 1) * (r - 2) / 2;

        if (!(SQUARES[blocksq] & board.pieces(ALLPIECES))) {

            const uint64_t path = FrontFileMask[side][sq];
            const uint64_t defended = (path & info.attackedSquares[side]) | (FrontFileMask[!side][sq] & (board.pieces(ROOK, side) | board.pieces(QUEEN, side)) & generateRookMoves(sq, ALLPIECES, 0));
            const uint64_t attacked = path & (info.attackedSquares[!side] | board.pieces(!side));
            const uint64_t push = SQUARES[blocksq] & ~board.pieces(ALLPIECES);

            if (push) {
                score += (passerPushBonus * rfactor);
                if (defended & SQUARES[blocksq]) {
                    score += (passerSafePushBonus * rfactor);
                }
            }

            if (!attacked) {
                score += (passerNoAttackersBonus * rfactor);
            }

            if (defended == path) {
                score += (passerFullyDefendedBonus * rfactor);
            }

            if (SQUARES[sq] & info.attackedSquares[!side]) {
                score -= (passerAttackedPenalty * rfactor);
            }

            //if (!(SQUARES[blocksq] & attacked)) {
            //    score += (passerBlockSqUndefendedBonus * rfactor);
            //}

        }

        score += pawnPassedBonus[r];
        score += passerFileBonus[f];
        score.eg += (5 * rfactor * kingDistance[info.kingSq[!side]][blocksq]) - (3 * rfactor * kingDistance[info.kingSq[side]][blocksq]);

    }

    return S(std::max(0, score.mg), std::max(0, score.eg));

}

static const Score evaluate_imbalances(const Board& board, const Side side) {

    Score score;

    for (unsigned int pt = 1; pt <= 4; pt++) {
        for (unsigned int pt2 = 0; pt2 < pt; pt2++) {
            const unsigned int own =  side + 2 + (pt  * 2);
            const unsigned int opp = !side + 2 + (pt2 * 2);
            const unsigned int mp  = board.piececount(own) * board.piececount(opp);
            score.mg += Imbalance[0][pt][pt2] * mp;
            score.eg += Imbalance[1][pt][pt2] * mp;
        }
    }

    if (board.piececount(Bishop(side)) >= 2) {
        score += bishopPairBonus;
    }

    if (board.piececount(Knight(side)) >= 2) {
        score -= knightPairPenalty;
    }

    if (board.piececount(Rook(side)) >= 2) {
        score -= rookPairPenalty;
    }

    return score;

}

static const Score evaluate_threats(const Board& board, const Side side, const EvalInfo& info) {

    Score score;

    const uint64_t undefendedPawns = board.pieces(PAWN, !side) & ~info.attackedSquares[!side];
    const uint64_t loosePawns = undefendedPawns & info.attackedSquares[side];
    const uint64_t safePawns  = (~undefendedPawns & board.pieces(PAWN, side)) & info.attackedSquares[side];
    const uint64_t weakPawns  = info.weakPawns & board.pieces(!side);

    const uint64_t minorsAndMajors = board.pieces(!side) & ~(board.pieces(KING, !side) | board.pieces(PAWN, !side));

    score += safePawnAttack  * popcount(generate_pawns_attacks(safePawns, side) & minorsAndMajors);
    score += loosePawnAttack * popcount(generate_pawns_attacks(loosePawns, side) & minorsAndMajors);
    score += loosePawnWeight * popcount(loosePawns);

    // Bonus for weak pawns that may be attacked by majors
    if (board.pieces(side, ROOK) || board.pieces(side, QUEEN)) {
        score += weakPawn * popcount(weakPawns);
    }

    uint64_t pawnPushes = 0;
    if (side == WHITE) {
        pawnPushes = (board.pieces(WHITE_PAWN) << 8) & ~board.pieces(ALLPIECES);
        pawnPushes |= ((pawnPushes & RANK_6) << 8) & ~board.pieces(ALLPIECES);
        pawnPushes &= ~info.attackedSquares[BLACK_PAWN] & (info.attackedSquares[side] | ~info.attackedSquares[!side]);
    } else {
        pawnPushes = (board.pieces(BLACK_PAWN) >> 8) & ~board.pieces(ALLPIECES);
        pawnPushes |= ((pawnPushes & RANK_3) >> 8) & ~board.pieces(ALLPIECES);
        pawnPushes &= ~info.attackedSquares[WHITE_PAWN] & (info.attackedSquares[side] | ~info.attackedSquares[!side]);
    }

    score += pawnPushThreat * popcount(generate_pawns_attacks(pawnPushes, side) & minorsAndMajors);

    const uint64_t weak = board.pieces(!side) & ~info.multiAttackedSquares[!side] & info.attackedSquares[side];
    uint64_t minorAttacks = board.pieces(!side) & (info.attackedSquares[Knight(side)] | info.attackedSquares[Bishop(side)]);
    uint64_t rookAttacks  = weak & info.attackedSquares[Rook(side)];

    while (minorAttacks) {

        const unsigned int sq = pop_lsb(minorAttacks);
        const PieceType pt = type(board.piecetype(sq));

        score += minorAttackWeight[pt / 2 - 1];
        if (pt != PAWN) {
            score += pieceVulnerable * relative_rank(!side, sq);
        }
    }

    while (rookAttacks) {
        const unsigned int sq = pop_lsb(rookAttacks);
        const PieceType pt = type(board.piecetype(sq));

        score += rookAttackWeight[pt / 2 - 1];
        if (pt != PAWN) {
            score += pieceVulnerable * relative_rank(!side, sq);
        }
    }

    score += loosePieceWeight * popcount(weak & ~info.attackedSquares[!side] & ~board.pieces(PAWN, !side));

    return score;

}

const int evaluate(const Board& board) {

    Score score;

    EvalInfo info;

    unsigned int pieceCount;

    // Material draw
    if ((pieceCount = popcount(board.pieces(ALLPIECES))) <= 4) {
        if (board.checkMaterialDraw(pieceCount) == true) return 0;
    }

    PawnEntry * pentry = pawnTable.probe(board.pawnkey());
    if (pentry != NULL) {
        score += pentry->score;
        info.weakPawns = pentry->weakPawns;
        info.passedPawns = pentry->passedPawns;
        info.attackedSquares[WHITE_PAWN] = pentry->pawnWAttacks;
        info.attackedSquares[BLACK_PAWN] = pentry->pawnBAttacks;
        info.pawnAttacksSpan[WHITE] = pentry->pawnWAttacksSpan;
        info.pawnAttacksSpan[BLACK] = pentry->pawnBAttacksSpan;
    } else {
        info.attackedSquares[WHITE_PAWN] = board.gen_wpawns_attacks();
        info.attackedSquares[BLACK_PAWN] = board.gen_bpawns_attacks();
    }

    info.mobilityArea[WHITE] = ALL_SQUARES & ~(board.pieces(WHITE_KING) | (board.pieces(WHITE_PAWN) & (board.pieces(ALLPIECES) >> 8 | RANKS[6] | RANKS[5])) | info.attackedSquares[BLACK_PAWN]);
    info.mobilityArea[BLACK] = ALL_SQUARES & ~(board.pieces(BLACK_KING) | (board.pieces(BLACK_PAWN) & (board.pieces(ALLPIECES) << 8 | RANKS[1] | RANKS[2])) | info.attackedSquares[WHITE_PAWN]);

    info.kingSq[WHITE] = lsb_index(board.pieces(WHITE_KING));
    info.kingSq[BLACK] = lsb_index(board.pieces(BLACK_KING));

    info.kingZone[WHITE] = KingZone[WHITE][info.kingSq[WHITE]];
    info.kingZone[BLACK] = KingZone[BLACK][info.kingSq[BLACK]];

    info.kingRing[WHITE] = KingRing[WHITE][info.kingSq[WHITE]];
    info.kingRing[BLACK] = KingRing[BLACK][info.kingSq[BLACK]];

    info.attackedSquares[WHITE] |= info.kingRing[WHITE] | info.attackedSquares[WHITE_PAWN];
    info.attackedSquares[BLACK] |= info.kingRing[BLACK] | info.attackedSquares[BLACK_PAWN];

    info.multiAttackedSquares[WHITE] = info.kingRing[WHITE] & info.attackedSquares[WHITE_PAWN];
    info.multiAttackedSquares[BLACK] = info.kingRing[BLACK] & info.attackedSquares[BLACK_PAWN];

    // Material
    score += board.material(WHITE);
    score -= board.material(BLACK);

    // Piece square tables
    score += board.pst(WHITE);
    score -= board.pst(BLACK);

    // Pawns
    if (pentry == NULL) {
        Score pawnScore = evaluate_pawns(board, WHITE, info) - evaluate_pawns(board, BLACK, info);
        pawnTable.store(board.pawnkey(), pawnScore, info.attackedSquares[WHITE_PAWN], info.attackedSquares[BLACK_PAWN], info.weakPawns, info.passedPawns, info.pawnAttacksSpan[WHITE], info.pawnAttacksSpan[BLACK]);
        score += pawnScore;
    }

    // Pieces
    score += evaluate_knights(board, WHITE, info);
    score += evaluate_bishops(board, WHITE, info);
    score += evaluate_rooks(board, WHITE, info);
    score += evaluate_queens(board, WHITE, info);

    score -= evaluate_knights(board, BLACK, info);
    score -= evaluate_bishops(board, BLACK, info);
    score -= evaluate_rooks(board, BLACK, info);
    score -= evaluate_queens(board, BLACK, info);

    // Mobility
    score += info.mobility[WHITE];
    score -= info.mobility[BLACK];

    // King Safety
    score += evaluate_king_safety(board, WHITE, info);
    score -= evaluate_king_safety(board, BLACK, info);

    // Add king attacks to attackedSquares
    info.attackedSquares[WHITE] |= info.kingRing[WHITE];
    info.attackedSquares[BLACK] |= info.kingRing[BLACK];

    // Passed Pawns
    score += evaluate_passers(board, WHITE, info);
    score -= evaluate_passers(board, BLACK, info);

    // Threats
    score += evaluate_threats(board, WHITE, info);
    score -= evaluate_threats(board, BLACK, info);

    // Imbalances
    MaterialEntry * mentry = materialTable.probe(board.materialkey());
    if (mentry != NULL) {
        score += mentry->score;
    } else {
        Score imbalanceScore = evaluate_imbalances(board, WHITE) - evaluate_imbalances(board, BLACK);
        materialTable.store(board.materialkey(), imbalanceScore);
        score += imbalanceScore;
    }

    return ((board.turn() == WHITE) ? scaled_eval(board.scale(), score) : -scaled_eval(board.scale(), score)) + tempoBonus;

}

void evaluateInfo(const Board& board) {

    Score score;

    EvalInfo info;

    info.attackedSquares[WHITE_PAWN] = board.gen_wpawns_attacks();
    info.attackedSquares[BLACK_PAWN] = board.gen_bpawns_attacks();

    info.mobilityArea[WHITE] = ALL_SQUARES & ~(board.pieces(WHITE_KING) | (board.pieces(WHITE_PAWN) & (board.pieces(ALLPIECES) >> 8 | RANKS[6] | RANKS[5])) | info.attackedSquares[BLACK_PAWN]);
    info.mobilityArea[BLACK] = ALL_SQUARES & ~(board.pieces(BLACK_KING) | (board.pieces(BLACK_PAWN) & (board.pieces(ALLPIECES) << 8 | RANKS[1] | RANKS[2])) | info.attackedSquares[WHITE_PAWN]);

    info.kingSq[WHITE] = lsb_index(board.pieces(WHITE_KING));
    info.kingSq[BLACK] = lsb_index(board.pieces(BLACK_KING));

    info.kingZone[WHITE] = KingZone[WHITE][info.kingSq[WHITE]];
    info.kingZone[BLACK] = KingZone[BLACK][info.kingSq[BLACK]];

    info.kingRing[WHITE] = KingRing[WHITE][info.kingSq[WHITE]];
    info.kingRing[BLACK] = KingRing[BLACK][info.kingSq[BLACK]];

    info.attackedSquares[WHITE] |= info.kingRing[WHITE] | info.attackedSquares[WHITE_PAWN];
    info.attackedSquares[BLACK] |= info.kingRing[BLACK] | info.attackedSquares[BLACK_PAWN];

    info.multiAttackedSquares[WHITE] = info.kingRing[WHITE] & info.attackedSquares[WHITE_PAWN];
    info.multiAttackedSquares[BLACK] = info.kingRing[BLACK] & info.attackedSquares[BLACK_PAWN];

    // Material
    const Score whiteMaterialPsqt = board.material(WHITE) + board.pst(WHITE);
    const Score blackMaterialPsqt = board.material(BLACK) + board.pst(BLACK);

    // Pawns
    const Score whitePawns = evaluate_pawns(board, WHITE, info);
    const Score blackPawns = evaluate_pawns(board, BLACK, info);

    // Pieces
    const Score whiteKnights = evaluate_knights(board, WHITE, info);
    const Score whiteBishops = evaluate_bishops(board, WHITE, info);
    const Score whiteRooks   = evaluate_rooks(board, WHITE, info);
    const Score whiteQueens  = evaluate_queens(board, WHITE, info);

    const Score blackKnights = evaluate_knights(board, BLACK, info);
    const Score blackBishops = evaluate_bishops(board, BLACK, info);
    const Score blackRooks   = evaluate_rooks(board, BLACK, info);
    const Score blackQueens  = evaluate_queens(board, BLACK, info);

    // King Safety
    const Score whiteKingSafety = evaluate_king_safety(board, WHITE, info);
    const Score blackKingSafety = evaluate_king_safety(board, BLACK, info);

    // Add king attacks to attackedSquares
    info.attackedSquares[WHITE] |= info.kingRing[WHITE];
    info.attackedSquares[BLACK] |= info.kingRing[BLACK];

    // Passed Pawns
    const Score whitePassers = evaluate_passers(board, WHITE, info);
    const Score blackPassers = evaluate_passers(board, BLACK, info);

    // Threats
    const Score whiteThreats = evaluate_threats(board, WHITE, info);
    const Score blackThreats = evaluate_threats(board, BLACK, info);

    // Imbalances
    const Score whiteImbalances = evaluate_imbalances(board, WHITE);
    const Score blackImbalances = evaluate_imbalances(board, BLACK);

    // Output evaluation
    std::cout << "(White)" << std::endl;
    std::cout << "Material & Psqt : " << whiteMaterialPsqt.mg << " | " << whiteMaterialPsqt.eg << std::endl;
    std::cout << "Imbalance       : " << whiteImbalances.mg << " | " << whiteImbalances.eg << std::endl;
    std::cout << "Pawns           : " << whitePawns.mg << " | " << whitePawns.eg << std::endl;
    std::cout << "Knights         : " << whiteKnights.mg << " | " << whiteKnights.eg << std::endl;
    std::cout << "Bishops         : " << whiteBishops.mg << " | " << whiteBishops.eg << std::endl;
    std::cout << "Rooks           : " << whiteRooks.mg << " | " << whiteRooks.eg << std::endl;
    std::cout << "Queens          : " << whiteQueens.mg << " | " << whiteQueens.eg << std::endl;
    std::cout << "Mobility        : " << info.mobility[WHITE].mg << " | " << info.mobility[WHITE].eg << std::endl;
    std::cout << "Passed Pawns    : " << whitePassers.mg << " | " << whitePassers.eg << std::endl;
    std::cout << "King safety     : " << whiteKingSafety.mg << " | " << whiteKingSafety.eg << std::endl;
    std::cout << "Threats         : " << whiteThreats.mg << " | " << whiteThreats.eg << std::endl;

    std::cout << std::endl;

    std::cout << "(Black)" << std::endl;
    std::cout << "Material & Psqt : " << blackMaterialPsqt.mg << " | " << blackMaterialPsqt.eg << std::endl;
    std::cout << "Imbalance       : " << blackImbalances.mg << " | " << blackImbalances.eg << std::endl;
    std::cout << "Pawns           : " << blackPawns.mg << " | " << blackPawns.eg << std::endl;
    std::cout << "Knights         : " << blackKnights.mg << " | " << blackKnights.eg << std::endl;
    std::cout << "Bishops         : " << blackBishops.mg << " | " << blackBishops.eg << std::endl;
    std::cout << "Rooks           : " << blackRooks.mg << " | " << blackRooks.eg << std::endl;
    std::cout << "Queens          : " << blackQueens.mg << " | " << blackQueens.eg << std::endl;
    std::cout << "Mobility        : " << info.mobility[BLACK].mg << " | " << info.mobility[BLACK].eg << std::endl;
    std::cout << "Passed Pawns    : " << blackPassers.mg << " | " << blackPassers.eg << std::endl;
    std::cout << "King safety     : " << blackKingSafety.mg << " | " << blackKingSafety.eg << std::endl;
    std::cout << "Threats         : " << blackThreats.mg << " | " << blackThreats.eg << std::endl;

    std::cout << std::endl;

    score += whiteMaterialPsqt - blackMaterialPsqt;
    score += whiteImbalances - blackImbalances;
    score += whiteKnights - blackKnights;
    score += whiteBishops - blackBishops;
    score += whiteRooks - blackRooks;
    score += whiteQueens - blackQueens;
    score += whitePawns - blackPawns;
    score += info.mobility[WHITE] - info.mobility[BLACK];
    score += whitePassers - blackPassers;
    score += whiteKingSafety - blackKingSafety;
    score += whiteThreats - blackThreats;

    int finalScore = scaled_eval(board.scale(), score);
    int normalEval = evaluate(board);

    if (std::abs(finalScore + (board.turn() == WHITE ? tempoBonus : -tempoBonus)) != std::abs(normalEval)) {
        std::cout << "ERROR: Difference between evaluation functions!!!" << std::endl;
        std::cout << (finalScore + (board.turn() == WHITE ? tempoBonus : -tempoBonus)) << std::endl;
        std::cout << normalEval - tempoBonus << std::endl;
        abort();
    }

    std::cout << "Total(For White): " << scaled_eval(board.scale(), score) << std::endl;

}
