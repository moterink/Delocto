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

static const uint64_t OutpostSquares[2] = { RANK_4 | RANK_5 | RANK_6, RANK_5 | RANK_4 | RANK_3 };
static const Score OutpostBonus[2]          = { S(34, 11), S(17, 6) };
static const Score OutpostReachableBonus[2] = { S(17,  6), S( 8, 3) };

static const Score Mobility[4][28] = {

    // Knights
    { S(-29, -35), S(-22, -25), S(-5, -12), S(-2, -6), S(2, 4), S(6, 8), S(9, 11), S(12, 14), S(14, 15) },
    // Bishops
    { S(-21, -32), S(-9, -11), S(8, 1), S(12, 6), S(17, 11), S(21, 16), S(24, 23), S(29, 27), S(30, 31), S(32, 34), S(37, 36), S(38, 39), S(41, 40), S(44, 43) },
    // Rooks
    { S(-27, -36), S(-13, -8), S(-7, 12), S(-4, 19), S(-3, 25), S(-1, 36), S(4, 45), S(8, 47), S(12, 53), S(12, 57), S(14, 63), S(15, 65), S(17, 67), S(20, 68), S(23, 68) },
    // Queens
    { S(-25, -40), S(-8, -2), S(0, 6), S(1, 9), S(5, 18), S(8, 23), S(12, 28), S(15, 32), S(16, 34), S(19, 38), S(21, 42), S(24, 45), S(25, 46), S(27, 46), S(27, 49), S(28, 51), S(29, 52), S(30, 54), S(32, 56), S(34, 58), S(35, 62), S(39, 66), S(40, 70), S(40, 75), S(41, 79), S(43, 81), S(44, 83), S(45, 84) }

};

// Pawns
static const Score pawnDoubledPenalty           = S(8, 14);
static const Score pawnIsolatedPenalty[2]       = { S(11, 12), S(5, 7) };
static const Score pawnBackwardPenalty[2]       = { S(17, 11), S(10, 5) };
static const Score pawnLeverBonus[8]            = { S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(7, 6), S(13, 13), S(0, 0), S(0, 0) };
static const int pawnConnectedBonus[8]  = {

    0, 3, 4, 6, 14, 23, 40, 0

};
static const Score pawnPhalanxBonus[2][8]  = {

    { S(0, 0), S(8, 0), S(9, 0), S(16, 2), S(37, 18), S(57, 43), S(105, 105), S(0, 0) },
    { S(0, 0), S(4, 0), S(4, 0), S( 9, 1), S(18,  9), S(29, 21), S( 52,  52), S(0, 0) }

};
static const Score pawnSupportBonus             = S(6, 2);
static const Score pawnPassedRankBonus[8]       = { S(0, 0), S(2, 6), S(4, 8), S(6, 14), S(28, 31), S(72, 78), S(115, 110), S(0, 0) };
static const Score pawnPassedFileBonus[8]       = { S(1, 4), S(0, 3), S(-4, -3), S(-12, -6), S(-12, -6), S(-4, -3), S(0, 3), S(1, 4) };
static const Score pawnCandidateBonus[8]        = { S(0, 0), S(0, 0), S(5, 15),  S(10, 30), S(20, 45), S(30, 50),   S(40, 60),   S(0, 0) };

static const Score passedPawnNoAttacks          = S(13, 15);
static const Score passedPawnSafePath           = S( 6,  7);
static const Score passedPawnSafePush           = S( 3,  4);
static const Score passedPawnBlockSqDefended    = S( 1,  2);

// Knights
static const Score knightPairPenalty            = S(13, 14);

// Bishops
static const Score bishopPairBonus              = S(32, 37);
static const Score bishopPawnsSameColorPenalty  = S( 1,  3);
static const Score bishopCenterAlignBonus       = S(21,  0);

// Minors
static const unsigned int outpostProtectorBonus[2] = { 6, 3 };
static const Score OutpostReachable[2]             = { S(8, 0), S(4, 0) };
static const Score minorPawnShield                 = S(8, 1);

// Rooks
static const Score rookOpenFileBonus            = S(19,  9);
static const Score rookSemiOpenFileBonus        = S( 6,  4);
static const Score rookPawnAlignBonus           = S( 3, 12);
static const unsigned int rookTrappedPenalty    = 40;
static const Score rookPairPenalty              = S(20, 16);

// King
static const unsigned int queenSafeCheckWeight       = 360;
static const unsigned int rookSafeCheckWeight        = 510;
static const unsigned int bishopSafeCheckWeight      = 290;
static const unsigned int knightSafeCheckWeight      = 380;
static const unsigned int kingUnsafeCheck            =  65;
static const unsigned int kingRingAttackWeight       =  33;
static const unsigned int kingRingWeakSquareAttack   =  88;
static const unsigned int kingSliderBlocker          =  65;
static const unsigned int kingKnightDefender         =  47;
static const unsigned int kingBishopDefender         =  18;
static const unsigned int kingNoQueenAttacker        = 410;
static const unsigned int attackerWeight[4]          = { 37, 25, 21, 6 };
static const Score kingPawnlessFlank                 = S(8, 45);
static const Score kingFlankAttack                   = S(4,  0);
static const Score kingProtectorDistancePenalty      = S(3,  4);

static const int kingPawnShelterValues[4][8] = {

    {  -3, 38,  44,  27,  18,   8,  12, 0 },
    { -20, 29,  16, -23, -14,  -5, -30, 0 },
    {  -5, 35,  11,  -1,  15,   1, -21, 0 },
    { -18, -6, -14, -24, -23, -31, -78, 0 }

};

static const int kingPawnStormValues[4][8] = {

    { 42, -134, -87, 44, 27, 21, 24, 0 },
    { 21,   -8,  58, 22, 18, -3, 11, 0 },
    {  2,   24,  76, 17,  3, -7, -1, 0 },
    { -5,   -7,  42,  7,  1, -3, -8, 0 }

};

// Threats
static const Score safePawnAttack          = S(85, 46);
static const Score loosePawnAttack         = S(15,  6);
static const Score loosePawnWeight         = S(16, 28);
static const Score loosePieceWeight        = S(23, 14);
static const Score pawnPushThreat          = S(20, 12);
static const Score pieceVulnerable         = S( 5,  0);
static const Score mobilityRestriction     = S( 3,  3);
static const Score KnightQueenAttackThreat = S( 8,  6);
static const Score BishopQueenAttackThreat = S(28,  8);
static const Score RookQueenAttackThreat   = S(28,  8);
static const Score minorAttackWeight[5] = {
    S(0, 15), S(19, 20), S(28, 22), S(34, 55), S(30, 59)
};
static const Score rookAttackWeight[5] = {
    S(0, 11), S(18, 34), S(16, 32), S(0, 17), S(25, 19)
};

// Tempo Bonus
static const int tempoBonus = 12;

int kingDistance[64][64];
static int kingPawnShelter[8][8];
static int kingPawnStorm[8][8];

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

    // Mirror King Pawn Shelter Values
    for (unsigned int f = 0; f < 4; f++) {
        for (unsigned int r = 0; r < 8; r++) {
            kingPawnShelter[f][r]     = kingPawnShelterValues[f][r];
            kingPawnShelter[7 - f][r] = kingPawnShelterValues[f][r];
        }
    }

    // Mirror King Pawn Storm Values
    for (unsigned int f = 0; f < 4; f++) {
        for (unsigned int r = 0; r < 8; r++) {
            kingPawnStorm[f][r]       = kingPawnStormValues[f][r];
            kingPawnStorm[7 - f][r]   = kingPawnStormValues[f][r];
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

    const uint64_t kingAttacks = moves & info.kingRing[!side];

    if (kingAttacks) {

        info.kingAttackersWeight[!side] += attackerWeight[pindex];
        info.kingAttackersNum[!side]++;
        info.kingRingAttacks[!side] += popcount(kingAttacks);

    }

}

static const Score evaluate_knights(const Board& board, const Side side, EvalInfo& info) {

    Score score;

    uint64_t knights = board.pieces(KNIGHT, side);
    while (knights) {

        unsigned int sq = pop_lsb(knights);
        uint64_t moves = generateKnightMoves(sq, 0);

        uint64_t outposts = OutpostSquares[side] & info.attackedSquares[Pawn(side)] & ~info.pawnAttacksSpan[!side];
        if (outposts & SQUARES[sq]) {
            score += OutpostBonus[0];
        } else if (outposts & moves & ~board.pieces(side)) {
            score += OutpostReachableBonus[0];
        }

        if (SQUARES[sq] & shift_down(board.pieces(WHITE_PAWN) | board.pieces(BLACK_PAWN), side)) {
            score += minorPawnShield;
        }

        score -= kingProtectorDistancePenalty * kingDistance[sq][info.kingSq[side]];

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

        uint64_t outposts = OutpostSquares[side] & info.attackedSquares[Pawn(side)] & ~info.pawnAttacksSpan[!side];
        if (outposts & SQUARES[sq]) {
            score += OutpostBonus[1];
        } else if (outposts & moves & ~board.pieces(side)) {
            score += OutpostReachableBonus[1];
        }

        if (SQUARES[sq] & shift_down(board.pieces(WHITE_PAWN) | board.pieces(BLACK_PAWN), side)) {
            score += minorPawnShield;
        }

        uint64_t pawnsOnSameColor = board.get_same_colored_squares(sq) & board.pieces(PAWN, side);
        score -= bishopPawnsSameColorPenalty * popcount(pawnsOnSameColor) * (1 + popcount(info.blockedPawns[side] & CENTRAL_FILES));

        if (popcount(generateBishopMoves(sq, board.pieces(PAWN, side), 0) & CENTRAL_SQUARES) > 1) {
            score += bishopCenterAlignBonus;
        }

        score -= kingProtectorDistancePenalty * kingDistance[sq][info.kingSq[side]];

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
        const int f  = file(sq);
        const int r  = relative_rank(side, sq);

        const uint64_t front      = FrontFileMask[side][sq];
        const uint64_t neighbours = ADJ_FILES[f] & ownPawns;
        const uint64_t stoppers   = PassedPawnMask[side][sq] & oppPawns;

        info.pawnAttacksSpan[side] |= PawnAttacksSpan[side][sq];

        const bool doubled       = front & ownPawns;
        const bool opposed       = front & oppPawns;
        const bool lever         = AttackBitboards[Pawn(side)][sq] & oppPawns;
        const bool isolated      = !neighbours;
        const bool passed        = !stoppers;
        const uint64_t supported = (neighbours & RANKS[rank(sq + DIRECTIONS[side][DOWN])]);
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

        if (phalanx || supported) {
            int bonus = pawnConnectedBonus[r] * (phalanx ? 3 : 2) / (opposed ? 2 : 1) + 8 * popcount(supported);
            score += S(bonus, bonus * (r - 2) / 4);
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

    }

    return score;

}

static const Score evaluate_king_safety(const Board& board, const Side side, const EvalInfo& info) {

    Score score = S(0, 0);
    int pawnScore = 0;

    const uint64_t notBehind = ~KingShelterSpan[!side][info.kingSq[side]];
    const uint64_t ourPawns  = board.pieces(PAWN, side) & notBehind;
    const uint64_t oppPawns  = board.pieces(PAWN, !side) & notBehind;
    const int kingFile       = file(info.kingSq[side]);
    const int centralFile    = std::max(1, std::min(kingFile, 6));

    for (unsigned int f = centralFile - 1; f <= centralFile + 1; f++) {

        uint64_t owns = FILES[f] & ourPawns;
        uint64_t opps = FILES[f] & oppPawns;

        unsigned int ownRank = (owns ? relative_rank(side, lsb_index(most_backward(side, owns))) : 0);
        unsigned int oppRank = (opps ? relative_rank(side, lsb_index(most_forward(!side, opps))) : 0);

        pawnScore += kingPawnShelter[f][ownRank];
        pawnScore -= kingPawnStorm[f][oppRank];

    }

    if (!(board.pieces(Pawn(side)) & KING_FLANK[kingFile])) {
        score -= kingPawnlessFlank;
    }

    const uint64_t flankAttackedSquares = KING_FLANK[kingFile] & (ALL_SQUARES ^ COLOUR_BASE_SQUARES[!side]) & info.attackedSquares[!side];
    const unsigned int flankAttacksCount = popcount(flankAttackedSquares) + popcount(flankAttackedSquares & info.multiAttackedSquares[!side]);

    score -= kingFlankAttack * flankAttacksCount;

    const uint64_t ring = KingRing[side][info.kingSq[side]];

    const uint64_t weakSquares = (info.attackedSquares[!side] & ~info.multiAttackedSquares[side]) & (~info.attackedSquares[side] | info.attackedSquares[Queen(side)] | ring);
    const uint64_t safeSquares = ~board.pieces(!side) & (~info.attackedSquares[side] | (weakSquares & info.multiAttackedSquares[!side]));

    const uint64_t knightCheckSquares = generateKnightMoves(info.kingSq[side], board.pieces(side));
    const uint64_t bishopCheckSquares = generateBishopMoves(info.kingSq[side], board.pieces(ALLPIECES) ^ board.pieces(Queen(side)), 0);
    const uint64_t rookCheckSquares   = generateRookMoves(info.kingSq[side], board.pieces(ALLPIECES) ^ board.pieces(Queen(side)), 0);

    uint64_t unsafeChecks = 0;
    const uint64_t queenChecks  = info.attackedSquares[Queen(!side)] & (bishopCheckSquares | rookCheckSquares)  & ~info.attackedSquares[Queen(side)];
    const uint64_t rookChecks   = info.attackedSquares[Rook(!side)] & rookCheckSquares;
    const uint64_t bishopChecks = info.attackedSquares[Bishop(!side)] & bishopCheckSquares;
    const uint64_t knightChecks = info.attackedSquares[Knight(!side)] & knightCheckSquares;

    int danger = 0;

    if (!board.pieces(QUEEN, !side)) {
        danger -= kingNoQueenAttacker;
    }

    if (queenChecks & (safeSquares & ~rookChecks)) {
        danger += queenSafeCheckWeight;
    }

    if (rookChecks & safeSquares) {
        danger += rookSafeCheckWeight;
    } else {
        unsafeChecks |= rookChecks;
    }

    if (bishopChecks & (safeSquares & ~queenChecks)) {
        danger += bishopSafeCheckWeight;
    } else {
        unsafeChecks |= bishopChecks;
    }

    if (knightChecks & safeSquares) {
        danger += knightSafeCheckWeight;
    } else {
        unsafeChecks |= knightChecks;
    }

    unsafeChecks &= info.mobilityArea[!side];

    /*std::cout << side << std::endl;
    std::cout << danger << std::endl << std::endl;
    std::cout << (info.kingAttackersNum[side] * info.kingAttackersWeight[side]) << std::endl;
    std::cout << (kingRingAttackWeight * info.kingRingAttacks[side]) << std::endl;
    std::cout << kingRingWeakSquareAttack * popcount(ring & weakSquares) << std::endl;
    std::cout << kingUnsafeCheck * popcount(unsafeChecks) << std::endl;
    std::cout << kingSliderBlocker * popcount(board.blockers_for_king(side)) << std::endl;
    std::cout << 2 * flankAttacksCount / 8 << std::endl;
    std::cout << (kingKnightDefender * popcount(ring & info.attackedSquares[Knight(side)])) << std::endl;
    std::cout << (kingBishopDefender * popcount(ring & info.attackedSquares[Bishop(side)])) << std::endl;
    std::cout << (6 * pawnScore / 9) << std::endl << std::endl;
    std::cout << danger << ":" << std::max(0, (danger * danger / 2048)) << std::endl << std::endl;*/

    danger += (info.kingAttackersNum[side] * info.kingAttackersWeight[side])
            + kingRingAttackWeight * info.kingRingAttacks[side]
            + kingRingWeakSquareAttack * popcount(ring & weakSquares)
            + kingUnsafeCheck * popcount(unsafeChecks)
            + kingSliderBlocker * popcount(board.get_king_blockers(side))
            + 2 * flankAttacksCount / 8
            - kingKnightDefender * popcount(ring & info.attackedSquares[Knight(side)])
            - kingBishopDefender * popcount(ring & info.attackedSquares[Bishop(side)])
            - 6 * pawnScore / 9;



    if (danger > 0) {
        score -= S(danger * danger / 2048, 0);
    }

    score.mg += pawnScore;

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
        const unsigned int rfactor = (r - 2) * (r - 2) / 2;

        score += S(0, ((5 * kingDistance[info.kingSq[!side]][blocksq]) - (2 * kingDistance[info.kingSq[side]][blocksq])) * rfactor);

        if (r > 2 && !(SQUARES[blocksq] & board.pieces(ALLPIECES))) {

            Score bonus = S(0, 0);

            uint64_t path = FrontFileMask[side][sq];
            uint64_t behind = FrontFileMask[!side][sq];
            uint64_t attacked = PawnAttacksSpan[side][sq] | path;
            uint64_t defended = FrontFileMask[side][sq];

            if (!((board.pieces(QUEEN, !side) | board.pieces(ROOK, !side)) & behind)) {
                attacked &= (info.attackedSquares[!side] | board.pieces(!side));
            }

            if (!((board.pieces(QUEEN, side) | board.pieces(ROOK, side)) & behind)) {
                defended &= info.attackedSquares[side];
            }

            if (defended & blocksq) {
                bonus += passedPawnBlockSqDefended;
            }

            if (!attacked) {
                bonus += passedPawnNoAttacks;
            } else if (!(attacked & path)) {
                bonus += passedPawnSafePath;
            } else if (!(attacked & blocksq)) {
                bonus += passedPawnSafePush;
            }

            score += bonus * rfactor;

        }

        score += pawnPassedRankBonus[r];
        score += pawnPassedFileBonus[f];

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

    const uint64_t stronglyProtectedSquares = info.attackedSquares[!side] & (info.multiAttackedSquares[!side] | info.attackedSquares[Pawn(!side)]);
    const uint64_t undefendedPawns = board.pieces(PAWN, !side) & ~info.attackedSquares[!side];
    const uint64_t loosePawns = undefendedPawns & info.attackedSquares[side];
    const uint64_t safePawns  = (~undefendedPawns & board.pieces(PAWN, side)) & info.attackedSquares[side];
    const uint64_t minorsAndMajors = board.pieces(!side) & ~(board.pieces(KING, !side) | board.pieces(PAWN, !side));

    score += safePawnAttack  * popcount(generate_pawns_attacks(safePawns, side) & minorsAndMajors);
    score += loosePawnAttack * popcount(generate_pawns_attacks(loosePawns, side) & minorsAndMajors);
    score += loosePawnWeight * popcount(loosePawns);

    uint64_t pawnPushes = shift_up(board.pieces(side, PAWN), side) & ~board.pieces(ALLPIECES);
    pawnPushes |= shift_up(pawnPushes & PAWN_FIRST_PUSH_RANK[side], side) & ~board.pieces(ALLPIECES);
    pawnPushes &= (~info.attackedSquares[Pawn(!side)] & (info.attackedSquares[side] | ~info.attackedSquares[!side]));
    score += pawnPushThreat * popcount(generate_pawns_attacks(pawnPushes, side) & minorsAndMajors);

    const uint64_t weak = board.pieces(!side) & ~info.multiAttackedSquares[!side] & info.attackedSquares[side];
    uint64_t minorAttacks = board.pieces(!side) & (info.attackedSquares[Knight(side)] | info.attackedSquares[Bishop(side)]);
    uint64_t rookAttacks  = weak & info.attackedSquares[Rook(side)];

    while (minorAttacks) {

        const unsigned int sq = pop_lsb(minorAttacks);
        const PieceType pt = type(board.piecetype(sq));

        score += minorAttackWeight[pt_index(pt)];
        if (pt != PAWN) {
            score += pieceVulnerable * relative_rank(!side, sq);
        }

    }

    while (rookAttacks) {

        const unsigned int sq = pop_lsb(rookAttacks);
        const PieceType pt = type(board.piecetype(sq));

        score += rookAttackWeight[pt_index(pt)];
        if (pt != PAWN) {
            score += pieceVulnerable * relative_rank(!side, sq);
        }

    }

    score += loosePieceWeight * popcount(weak & ~info.attackedSquares[!side] & ~board.pieces(PAWN, !side));

    score += mobilityRestriction * popcount(info.attackedSquares[!side] & ~stronglyProtectedSquares & info.attackedSquares[side]);

    // Evaluate possible safe attacks on queen
    uint64_t queens = board.pieces(QUEEN, !side);
    if (queens) {

        unsigned int sq = pop_lsb(queens);
        uint64_t knightAttackSquares = generateKnightMoves(sq, board.pieces(side)) & info.attackedSquares[Knight(side)];
        uint64_t bishopAttackSquares = generateBishopMoves(sq, board.pieces(ALLPIECES), 0) & info.attackedSquares[Bishop(side)];
        uint64_t rookAttackSquares   = generateRookMoves(sq, board.pieces(ALLPIECES), 0) & info.attackedSquares[Rook(side)];
        uint64_t safe = info.mobilityArea[side] & ~stronglyProtectedSquares;

        score += KnightQueenAttackThreat * popcount(knightAttackSquares & safe);
        score += BishopQueenAttackThreat * popcount(bishopAttackSquares & safe);
        score += RookQueenAttackThreat   * popcount(rookAttackSquares & safe);

    }

    return score;

}

const int evaluate(const Board& board) {

    Score score;

    EvalInfo info;

    unsigned int pieceCount;

    // Material draw
    if ((pieceCount = popcount(board.pieces(ALLPIECES))) <= 4) {
        if (board.checkMaterialDraw(pieceCount) == true)
            return 0;
    }

    PawnEntry * pentry = pawnTable.probe(board.pawnkey());
    if (pentry != NULL) {
        score += pentry->score;
        info.passedPawns = pentry->passedPawns;
        info.attackedSquares[WHITE_PAWN] = pentry->pawnWAttacks;
        info.attackedSquares[BLACK_PAWN] = pentry->pawnBAttacks;
        info.pawnAttacksSpan[WHITE] = pentry->pawnWAttacksSpan;
        info.pawnAttacksSpan[BLACK] = pentry->pawnBAttacksSpan;
    } else {
        info.attackedSquares[WHITE_PAWN] = board.gen_wpawns_attacks();
        info.attackedSquares[BLACK_PAWN] = board.gen_bpawns_attacks();
    }

    info.mobilityArea[WHITE] = ALL_SQUARES & ~((board.pieces(WHITE_KING) | board.pieces(WHITE_QUEEN)) | (board.pieces(WHITE_PAWN) & ((board.pieces(ALLPIECES) >> 8) | RANKS[6] | RANKS[5])) | info.attackedSquares[BLACK_PAWN]);
    info.mobilityArea[BLACK] = ALL_SQUARES & ~((board.pieces(BLACK_KING) | board.pieces(BLACK_QUEEN)) | (board.pieces(BLACK_PAWN) & ((board.pieces(ALLPIECES) << 8) | RANKS[1] | RANKS[2])) | info.attackedSquares[WHITE_PAWN]);

    info.kingSq[WHITE] = lsb_index(board.pieces(WHITE_KING));
    info.kingSq[BLACK] = lsb_index(board.pieces(BLACK_KING));

    info.kingRing[WHITE] = KingRing[WHITE][info.kingSq[WHITE]];
    info.kingRing[BLACK] = KingRing[BLACK][info.kingSq[BLACK]];

    info.attackedSquares[WHITE] |= info.kingRing[WHITE] | info.attackedSquares[WHITE_PAWN];
    info.attackedSquares[BLACK] |= info.kingRing[BLACK] | info.attackedSquares[BLACK_PAWN];

    info.multiAttackedSquares[WHITE] = info.kingRing[WHITE] & info.attackedSquares[WHITE_PAWN];
    info.multiAttackedSquares[BLACK] = info.kingRing[BLACK] & info.attackedSquares[BLACK_PAWN];

    info.blockedPawns[WHITE] = (board.pieces(WHITE_PAWN) << 8) & board.pieces(ALLPIECES);
    info.blockedPawns[BLACK] = (board.pieces(BLACK_PAWN) >> 8) & board.pieces(ALLPIECES);

    // Material
    score += board.material(WHITE);
    score -= board.material(BLACK);

    // Piece square tables
    score += board.pst(WHITE);
    score -= board.pst(BLACK);

    // Pawns
    if (pentry == NULL) {
        Score pawnScore = evaluate_pawns(board, WHITE, info) - evaluate_pawns(board, BLACK, info);
        pawnTable.store(board.pawnkey(), pawnScore, info.attackedSquares[WHITE_PAWN], info.attackedSquares[BLACK_PAWN], info.passedPawns, info.pawnAttacksSpan[WHITE], info.pawnAttacksSpan[BLACK]);
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

    info.mobilityArea[WHITE] = ALL_SQUARES & ~((board.pieces(WHITE_KING) | board.pieces(WHITE_QUEEN)) | (board.pieces(WHITE_PAWN) & ((board.pieces(ALLPIECES) >> 8) | RANKS[6] | RANKS[5])) | info.attackedSquares[BLACK_PAWN]);
    info.mobilityArea[BLACK] = ALL_SQUARES & ~((board.pieces(BLACK_KING) | board.pieces(BLACK_QUEEN)) | (board.pieces(BLACK_PAWN) & ((board.pieces(ALLPIECES) << 8) | RANKS[1] | RANKS[2])) | info.attackedSquares[WHITE_PAWN]);

    info.kingSq[WHITE] = lsb_index(board.pieces(WHITE_KING));
    info.kingSq[BLACK] = lsb_index(board.pieces(BLACK_KING));

    info.kingRing[WHITE] = KingRing[WHITE][info.kingSq[WHITE]];
    info.kingRing[BLACK] = KingRing[BLACK][info.kingSq[BLACK]];

    info.attackedSquares[WHITE] |= info.kingRing[WHITE] | info.attackedSquares[WHITE_PAWN];
    info.attackedSquares[BLACK] |= info.kingRing[BLACK] | info.attackedSquares[BLACK_PAWN];

    info.multiAttackedSquares[WHITE] = info.kingRing[WHITE] & info.attackedSquares[WHITE_PAWN];
    info.multiAttackedSquares[BLACK] = info.kingRing[BLACK] & info.attackedSquares[BLACK_PAWN];

    info.blockedPawns[WHITE] = (board.pieces(WHITE_PAWN) << 8) & board.pieces(ALLPIECES);
    info.blockedPawns[BLACK] = (board.pieces(BLACK_PAWN) >> 8) & board.pieces(ALLPIECES);

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
