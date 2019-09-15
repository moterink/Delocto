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

#include "evaluate.hpp"
#include "movegen.hpp"
#include "uci.hpp"

Value Pst[14][64];

static const Value PstValues[6][32] = {

    // Pawns
    {
        V( 0,  0), V( 0,  0), V( 0,  0), V( 0,  0),
        V(-4, 2), V(4, -2), V(-5, 8), V(-2, 11),
        V(-5, 10), V(-7, 6), V(-3, 7), V(3, 14),
        V(4, 4), V(-3, 4), V(-4, -1), V(3, -6),
        V(-5, -1), V(-5, -3), V(6, -5), V(14, -4),
        V(-7, -3), V(-3, -4), V(8, -2), V(11, 2),
        V(-1, -7), V(2, -2), V(7, 4), V(9, 5),
        V( 0,  0), V( 0,  0), V( 0,  0), V( 0,  0)
    },
    // Knights
    {

        V(-94, -46), V(-38, -42), V(-25, -25), V(-15, -8),
        V(-31, -30), V(-10, -21), V(3, -17), V(17, 8),
        V(-5, -24), V(13, -18), V(30, -8), V(26, 9),
        V(-14, -19), V(6, -9), V(20, 2), V(24, 16),
        V(-13, -17), V(2, 0), V(19, 6), V(22, 16),
        V(-30, -18), V(-9, -15), V(2, -2), V(9, 13),
        V(-37, -33), V(-18, -26), V(-11, -7),  V(-4, 3),
        V(-79, -49), V(-45, -35), V(-38, -22), V(-37, -8),

    },
    // Bishops
    {

        V(-23, -24), V(-1, -19), V(-6, -18), V(-12, -9),
        V(-10, -16), V(-9, -8), V(5, -3), V(-3, 4),
        V(-8, -12), V(2, 1), V(0, 0), V(4, 8),
        V(-3, -11), V(14, -3), V(11, -5), V(13, 8),
        V(0, -12), V(4, -1), V(12, 0), V(17, 8),
        V(-4, -8), V(11, 0), V(-1, -3), V(7, 6),
        V(-8, -18), V(3, -6), V(7, -7), V(1, 0),
        V(-21, -30), V(-2, -14), V(-5, -16), V(-13, -4)

    },
    // Rooks
    {

        V(-10, 6), V(-11, -3), V(-3, 6), V(2, 3),
        V(-4, 0), V(3, 1), V(5, 8), V(6, -4),
        V(-11, 1), V(-2, -1), V(2, -5), V(5, 3),
        V(-11, -4), V(-6, 2), V(0, 2), V(3, -4),
        V(-6, -2), V(-2, 1), V(-2, -4), V(-3, 4),
        V(-10, 5), V(-3, -2), V(1, 1), V(0, -1),
        V(-8, -5), V(-5, -3), V(-2, 0), V(4, 0),
        V(-11, -1), V(-6, -3), V(-3, -1), V(1, -1)

    },
    // Queens
    {

        V(-1, -35), V(-1, -24), V(0, -20), V(-1, -17),
        V(-2, -23), V(3, -13), V(5, -11), V(4, -4),
        V(-2, -18), V(5, -8), V(3, -6), V(4, 0),
        V(0, -14), V(7, -3), V(6, 4), V(2, 10),
        V(2, -11), V(2, -1), V(4, 6), V(4, 11),
        V(-1, -18), V(3, -8), V(6, -4), V(3, 1),
        V(-1, -26), V(2, -15), V(4, -10), V(6, -2),
        V(1, -32), V(-2, -27), V(-2, -22), V(2, -12),

    },
    // King
    {

        V(30, 2), V(41, 28), V(23, 35), V(0, 35),
        V(41, 19), V(56, 46), V(30, 60), V(12, 66),
        V(56, 41), V(75, 77), V(40, 82), V(17, 89),
        V(68, 46), V(83, 78), V(53, 92), V(32, 91),
        V(79, 48), V(90, 71), V(64, 79), V(51, 79),
        V(93, 40), V(119, 65), V(79, 77), V(56, 81),
        V(130, 27), V(143, 46), V(112, 65), V(86, 62),
        V(128, 0), V(153, 19), V(128, 38), V(89, 44),

    }

};

static const int Imbalance[2][6][6] = {

    {
        { 42 },
        {  1, 1 },
        {  1, 7, -1 },
        {  0, 3,  0,  0 },
        { -1, 1,  3, -6 },
        { -5, 3,  4, -4, 0 }
    },
    {
        { 0 },
        { 1, 0 },
        { 0, 2,  0 },
        { 2, 2,  1,  0 },
        { 1, 1,  1, -1, 0 },
        { 3, 3, -1,  4, 9, 0 }
    }

};

static const uint64_t OutpostSquares[2]     = { RANK_5 | RANK_4 | RANK_3, RANK_4 | RANK_5 | RANK_6 };
static const Value OutpostBonus[2]          = { V(34, 11), V(17, 6) };
static const Value OutpostReachableBonus[2] = { V(17,  6), V( 8, 3) };

static const Value Mobility[4][28] = {

    // Knights
    { V(-29, -35), V(-22, -25), V(-5, -12), V(-2, -6), V(2, 4), V(6, 8), V(9, 11), V(12, 14), V(14, 15) },
    // Bishops
    { V(-21, -32), V(-9, -11), V(8, 1), V(12, 6), V(17, 11), V(21, 16), V(24, 23), V(29, 27), V(30, 31), V(32, 34), V(37, 36), V(38, 39), V(41, 40), V(44, 43) },
    // Rooks
    { V(-27, -36), V(-13, -8), V(-7, 12), V(-4, 19), V(-3, 25), V(-1, 36), V(4, 45), V(8, 47), V(12, 53), V(12, 57), V(14, 63), V(15, 65), V(17, 67), V(20, 68), V(23, 68) },
    // Queens
    { V(-25, -40), V(-8, -2), V(0, 6), V(1, 9), V(5, 18), V(8, 23), V(12, 28), V(15, 32), V(16, 34), V(19, 38), V(21, 42), V(24, 45), V(25, 46), V(27, 46), V(27, 49), V(28, 51), V(29, 52), V(30, 54), V(32, 56), V(34, 58), V(35, 62), V(39, 66), V(40, 70), V(40, 75), V(41, 79), V(43, 81), V(44, 83), V(45, 84) }

};

// Pawns
static const Value pawnDoubledPenalty           = V(8, 14);
static const Value pawnIsolatedPenalty[2]       = { V(11, 12), V(5, 7) };
static const Value pawnBackwardPenalty[2]       = { V(17, 11), V(10, 5) };
static const Value pawnLeverBonus[8]            = { V(0, 0), V(0, 0), V(0, 0), V(0, 0), V(7, 6), V(13, 13), V(0, 0), V(0, 0) };
static const int pawnConnectedBonus[8]  = {

    0, 3, 4, 6, 14, 23, 40, 0

};
static const Value pawnPhalanxBonus[2][8]  = {

    { V(0, 0), V(8, 0), V(9, 0), V(16, 2), V(37, 18), V(57, 43), V(105, 105), V(0, 0) },
    { V(0, 0), V(4, 0), V(4, 0), V( 9, 1), V(18,  9), V(29, 21), V( 52,  52), V(0, 0) }

};
static const Value pawnSupportBonus             = V(6, 2);
static const Value pawnPassedRankBonus[8]       = { V(0, 0), V(4, 13), V(8, 15), V(7, 19), V(29, 34), V(79, 83), V(130, 122), V(0, 0) };
static const Value pawnPassedFilePenalty[8]     = { V(0, 0), V(5, 4), V(10, 8), V(15, 12), V(15, 12), V(10, 8), V(5, 4), V(0, 0) };
static const Value pawnCandidateBonus[8]        = { V(0, 0), V(0, 0), V(5, 15),  V(10, 30), V(20, 45), V(30, 50),   V(40, 60),   V(0, 0) };

static const Value passedPawnNoAttacks          = V(16, 18);
static const Value passedPawnSafePath           = V( 9, 11);
static const Value passedPawnSafePush           = V( 4,  6);
static const Value passedPawnBlockSqDefended    = V( 2,  3);

// Bishops
static const Value bishopPawnsSameColorPenalty  = V( 1,  3);
static const Value bishopCenterAlignBonus       = V(21,  0);

// Minors
static const unsigned int outpostProtectorBonus[2] = { 6, 3 };
static const Value OutpostReachable[2]             = { V(8, 0), V(4, 0) };
static const Value minorPawnShield                 = V(8, 1);

// Rooks
static const Value rookOpenFileBonus      = V(19,  9);
static const Value rookSemiOpenFileBonus  = V( 6,  4);
static const Value rookPawnAlignBonus     = V( 3, 12);
static const Value rookTrappedPenalty     = V(22,  2);

// Queens
static const Value UnsafeQueen = V(23, 7);

// King
static const unsigned int queenSafeCheckWeight       = 365;
static const unsigned int rookSafeCheckWeight        = 505;
static const unsigned int bishopSafeCheckWeight      = 300;
static const unsigned int knightSafeCheckWeight      = 370;
static const unsigned int kingUnsafeCheck            =  65;
static const unsigned int kingRingAttackWeight       =  32;
static const unsigned int kingRingWeakSquareAttack   =  86;
static const unsigned int kingSliderBlocker          =  65;
static const unsigned int kingKnightDefender         =  47;
static const unsigned int kingBishopDefender         =  18;
static const unsigned int kingNoQueenAttacker        = 410;
static const unsigned int attackerWeight[4]          = { 36, 26, 21, 5 };
static const Value kingPawnlessFlank                 = V(8, 45);
static const Value kingFlankAttack                   = V(4,  0);
static const Value kingProtectorDistancePenalty      = V(3,  4);

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
static const Value safePawnAttack          = V(85, 46);
static const Value loosePawnWeight         = V(16, 28);
static const Value HangingPiece            = V(32, 17);
static const Value pawnPushThreat          = V(20, 12);
static const Value pieceVulnerable         = V( 6,  0);
static const Value mobilityRestriction     = V( 3,  3);
static const Value KnightQueenAttackThreat = V( 8,  6);
static const Value BishopQueenAttackThreat = V(28,  8);
static const Value RookQueenAttackThreat   = V(28,  8);
static const Value KingAttackThreat        = V(11, 42);
static const Value minorAttackWeight[5] = {
    V(0, 15), V(19, 20), V(28, 22), V(34, 55), V(30, 59)
};
static const Value rookAttackWeight[5] = {
    V(0, 11), V(18, 34), V(16, 32), V(0, 17), V(25, 19)
};

// Tempo Bonus
static const int tempoBonus = 12;

int kingDistance[64][64];
static int kingPawnShelter[8][8];
static int kingPawnStorm[8][8];

void init_king_distance() {

    unsigned int findex, tindex;

    for (findex = 0; findex < 64; findex++) {
        for (tindex = 0; tindex < 64; tindex++) {
            kingDistance[findex][tindex] = std::max(std::abs((int)rank(tindex) - (int)rank(findex)), std::abs((int)file(tindex) - (int)file(findex)));
        }
    }

}

void init_psqt() {

    for (unsigned int pt = PAWN; pt <= KING; pt+=2) {
        for (unsigned int sq = 0; sq < 32;  sq++) {
            const unsigned int r = sq / 4;
            const unsigned int f = sq & 0x3;
            Value value = PstValues[(pt - 2) / 2][sq];

            Pst[(pt | WHITE)][8 * r + f] = value;
            Pst[(pt | WHITE)][8 * r + (7 - f)] = value;
            Pst[(pt | BLACK)][8 * (7 - r) + f] = value;
            Pst[(pt | BLACK)][8 * (7 - r) + (7 - f)] = value;

        }
    }

}

void init_eval() {

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

static void update_attack_info(Color color, PieceType pt, unsigned int sq, uint64_t moves, EvalInfo& info) {

    unsigned int pindex = pt_index(pt) - 1;

    info.attackedSquares[pt] |= moves;
    info.multiAttackedSquares[color] |= info.attackedSquares[color] & moves;
    info.attackedSquares[color] |= moves;

    const uint64_t kingAttacks = moves & info.kingRing[!color];

    if (kingAttacks) {

        info.kingAttackersWeight[!color] += attackerWeight[pindex];
        info.kingAttackersNum[!color]++;
        info.kingRingAttacks[!color] += popcount(kingAttacks);

    }

}

static const Value evaluate_knights(const Board& board, const Color color, EvalInfo& info) {

    Value value;
    uint64_t knights = board.pieces(KNIGHT, color);
    while (knights) {

        unsigned int sq = pop_lsb(knights);
        uint64_t moves = generateKnightMoves(sq, 0);
        if (board.get_king_blockers(color) & SQUARES[sq]) {
            moves &= LineTable[sq][info.kingSq[color]];
        }

        uint64_t outposts = OutpostSquares[color] & info.attackedSquares[Pawn(color)] & ~info.pawnAttacksSpan[!color];
        if (outposts & SQUARES[sq]) {
            value += OutpostBonus[0];
        } else if (outposts & moves & ~board.pieces(color)) {
            value += OutpostReachableBonus[0];
        }

        if (SQUARES[sq] & shift_down(board.pieces(WHITE_PAWN) | board.pieces(BLACK_PAWN), color)) {
            value += minorPawnShield;
        }

        value -= kingProtectorDistancePenalty * kingDistance[sq][info.kingSq[color]];

        info.mobility[color] += Mobility[0][popcount(moves & info.mobilityArea[color])];
        update_attack_info(color, Knight(color), sq, moves, info);

    }

    return value;

}

static const Value evaluate_bishops(const Board& board, const Color color, EvalInfo& info) {

    Value value;

    uint64_t bishops = board.pieces(BISHOP, color);

    while (bishops) {

        unsigned int sq = pop_lsb(bishops);

        // Exclude queen for xrays
        uint64_t moves = generateBishopMoves(sq, board.pieces(ALLPIECES) & ~(board.pieces(QUEEN, color) | board.pieces(QUEEN, !color)), 0);
        if (board.get_king_blockers(color) & SQUARES[sq]) {
            moves &= LineTable[sq][info.kingSq[color]];
        }

        uint64_t outposts = OutpostSquares[color] & info.attackedSquares[Pawn(color)] & ~info.pawnAttacksSpan[!color];
        if (outposts & SQUARES[sq]) {
            value += OutpostBonus[1];
        } else if (outposts & moves & ~board.pieces(color)) {
            value += OutpostReachableBonus[1];
        }

        if (SQUARES[sq] & shift_down(board.pieces(WHITE_PAWN) | board.pieces(BLACK_PAWN), color)) {
            value += minorPawnShield;
        }

        uint64_t pawnsOnSameColor = board.get_same_colored_squares(sq) & board.pieces(PAWN, color);
        value -= bishopPawnsSameColorPenalty * popcount(pawnsOnSameColor) * (1 + popcount(info.blockedPawns[color] & CENTRAL_FILES));

        if (popcount(generateBishopMoves(sq, board.pieces(WHITE_PAWN) | board.pieces(BLACK_PAWN), 0) & CENTRAL_SQUARES) > 1) {
            value += bishopCenterAlignBonus;
        }

        value -= kingProtectorDistancePenalty * kingDistance[sq][info.kingSq[color]];

        info.mobility[color] += Mobility[1][popcount(moves & info.mobilityArea[color])];
        update_attack_info(color, Bishop(color), sq, moves, info);

    }

    return value;

}

static const Value evaluate_rooks(const Board& board, const Color color, EvalInfo& info) {

    Value value;

    uint64_t rooks = board.pieces(ROOK, color);
    while (rooks) {

        unsigned int sq = pop_lsb(rooks);

        // Exclude queens and rooks for xrays
        uint64_t moves = generateRookMoves(sq, board.pieces(ALLPIECES) & ~board.all_majors(), 0);
        if (board.get_king_blockers(color) & SQUARES[sq]) {
            moves &= LineTable[sq][info.kingSq[color]];
        }

        const unsigned int f = file(sq);
        const unsigned int mob = popcount(moves & info.mobilityArea[color]);

        if (!(FILES[f] & (board.pieces(WHITE_PAWN) | board.pieces(BLACK_PAWN)))) {
            value += rookOpenFileBonus;
        } else if (!(FILES[f] & board.pieces(PAWN, color))) {
            value += rookSemiOpenFileBonus;
        } else {

            const unsigned int kingFile = file(info.kingSq[color]);
            if (mob <= 3 && ((kingFile < 4) == (f < kingFile))) {
                value -= rookTrappedPenalty;
            }

        }

        // Bonus for aligning on file with enemy pawn
        if (relative_rank(color, sq) >= 4) {
            value += rookPawnAlignBonus * popcount(moves & board.pieces(PAWN, !color));
        }

        info.mobility[color] += Mobility[2][mob];
        update_attack_info(color, Rook(color), sq, moves, info);

    }

    return value;

}

static const Value evaluate_queens(const Board& board, const Color color, EvalInfo& info) {

    Value value;

    uint64_t queens = board.pieces(QUEEN, color);
    while (queens) {

        unsigned int sq = pop_lsb(queens);
        uint64_t moves = generateQueenMoves(sq, board.pieces(ALLPIECES), 0);
        if (board.get_king_blockers(color) & SQUARES[sq]) {
            moves &= LineTable[sq][info.kingSq[color]];
        }

        if (board.get_slider_blockers(board.pieces(Bishop(!color)) | board.pieces(Rook(!color)), sq)) {
            value -= UnsafeQueen;
        }

        info.mobility[color] += Mobility[3][popcount(moves & info.mobilityArea[color])];
        update_attack_info(color, Queen(color), sq, moves, info);

    }

    return value;

}

static const Value evaluate_pawns(const Board& board, const Color color, EvalInfo& info) {

    Value value;

    const uint64_t ownPawns = board.pieces(PAWN, color);
    const uint64_t oppPawns = board.pieces(PAWN, !color);
    const uint64_t allPawns = ownPawns | oppPawns;

    uint64_t pawns = ownPawns;

    while (pawns) {

        const unsigned int sq = pop_lsb(pawns);
        const int f  = file(sq);
        const int r  = relative_rank(color, sq);

        const uint64_t front      = FrontFileMask[color][sq];
        const uint64_t neighbours = ADJ_FILES[f] & ownPawns;
        const uint64_t stoppers   = PassedPawnMask[color][sq] & oppPawns;
        const uint64_t lever     = AttackBitboards[Pawn(color)][sq] & oppPawns;

        info.pawnAttacksSpan[color] |= PawnAttacksSpan[color][sq];

        const bool doubled       = front & ownPawns;
        const bool opposed       = front & oppPawns;
        const bool isolated      = !neighbours;
        const bool passed        = !(stoppers ^ lever);
        const uint64_t supported = (neighbours & RANKS[rank(sq + DIRECTIONS[color][DOWN])]);
        const uint64_t phalanx   = (neighbours & RANKS[rank(sq)]);

        bool backward = false;

        if (!isolated && !phalanx && r <= 4 && !lever) {
            const uint64_t br = RANKS[rank(lsb_index(most_backward(color, neighbours | stoppers)))];
            backward = (br | ((color == WHITE) ? (ADJ_FILES[f] & br) << 8 : (ADJ_FILES[f] & br) >> 8)) & stoppers;
        }

        if (doubled) {
            value -= pawnDoubledPenalty;
        } else if (passed) {
            info.passedPawns |= SQUARES[sq];
        }

        if (phalanx || supported) {
            int bonus = pawnConnectedBonus[r] * (phalanx ? 3 : 2) / (opposed ? 2 : 1) + 8 * popcount(supported);
            value += V(bonus, bonus * (r - 2) / 4);
        }

        if (phalanx) {
            value += pawnPhalanxBonus[opposed][r];
        } else if (isolated) {
            value -= pawnIsolatedPenalty[opposed];

        } else if (backward) {
            value -= pawnBackwardPenalty[opposed];
        }

        if (lever) {
            value += pawnLeverBonus[r];
        }

    }

    return value;

}

static const int evaluate_shelter_storm(const Board& board, const Color color, const EvalInfo& info, const unsigned int kingSq, const int oldValue) {

    int value = 0;

    const uint64_t notBehind = ~KingShelterSpan[!color][kingSq];
    const uint64_t ourPawns  = board.pieces(PAWN, color) & notBehind;
    const uint64_t oppPawns  = board.pieces(PAWN, !color) & notBehind;
    const int kingFile       = file(kingSq);
    const int centralFile    = std::max(1, std::min(kingFile, 6));

    for (unsigned int f = centralFile - 1; f <= centralFile + 1; f++) {

        uint64_t owns = FILES[f] & ourPawns;
        uint64_t opps = FILES[f] & oppPawns;

        unsigned int ownRank = (owns ? relative_rank(color, lsb_index(most_backward(color, owns))) : 0);
        unsigned int oppRank = (opps ? relative_rank(color, lsb_index(most_forward(!color, opps))) : 0);

        value += kingPawnShelter[f][ownRank];
        value -= kingPawnStorm[f][oppRank];

    }

    if (oldValue > value) {
        return oldValue;
    }

    return value;

}

static const Value evaluate_king_safety(const Board& board, const Color color, const EvalInfo& info) {

    Value value = V(0, 0);
    int pawnValue = evaluate_shelter_storm(board, color, info, info.kingSq[color], -VALUE_INFINITE);

    // Idea from Stockfish
    if (board.can_castle(CASTLE_FLAGS[color * 2])) {
        pawnValue = evaluate_shelter_storm(board, color, info, CASTLE_SQUARES[color * 2], pawnValue);
    }
    if (board.can_castle(CASTLE_FLAGS[color * 2 + 1])) {
        pawnValue = evaluate_shelter_storm(board, color, info, CASTLE_SQUARES[color * 2 + 1], pawnValue);
    }

    const int kingFile = file(info.kingSq[color]);
    const uint64_t flankAttackedSquares = KING_FLANK[kingFile] & (ALL_SQUARES ^ COLOUR_BASE_SQUARES[!color]) & info.attackedSquares[!color];
    const unsigned int flankAttacksCount = popcount(flankAttackedSquares) + popcount(flankAttackedSquares & info.multiAttackedSquares[!color]);

    const uint64_t ring = KingRing[color][info.kingSq[color]];

    const uint64_t weakSquares = (info.attackedSquares[!color] & ~info.multiAttackedSquares[color]) & (~info.attackedSquares[color] | info.attackedSquares[Queen(color)] | info.attackedSquares[King(color)]);
    const uint64_t safeSquares = ~board.pieces(!color) & (~info.attackedSquares[color] | (weakSquares & info.multiAttackedSquares[!color]));

    const uint64_t knightCheckSquares = generateKnightMoves(info.kingSq[color], board.pieces(color));
    const uint64_t bishopCheckSquares = generateBishopMoves(info.kingSq[color], board.pieces(ALLPIECES) ^ board.pieces(Queen(color)), 0);
    const uint64_t rookCheckSquares   = generateRookMoves(info.kingSq[color], board.pieces(ALLPIECES) ^ board.pieces(Queen(color)), 0);

    uint64_t unsafeChecks = 0;
    const uint64_t queenChecks  = info.attackedSquares[Queen(!color)] & (bishopCheckSquares | rookCheckSquares)  & ~info.attackedSquares[Queen(color)];
    const uint64_t rookChecks   = info.attackedSquares[Rook(!color)] & rookCheckSquares;
    const uint64_t bishopChecks = info.attackedSquares[Bishop(!color)] & bishopCheckSquares;
    const uint64_t knightChecks = info.attackedSquares[Knight(!color)] & knightCheckSquares;

    int danger = 0;

    if (!board.pieces(QUEEN, !color)) {
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

    unsafeChecks &= info.mobilityArea[!color];

    /*std::cout << "Color: " << color << std::endl;
    std::cout << "Safe Checks & No Queen: " << danger << std::endl;*/

    danger += info.kingAttackersNum[color] * info.kingAttackersWeight[color]
            + kingRingAttackWeight * info.kingRingAttacks[color]
            + kingRingWeakSquareAttack * popcount(ring & weakSquares)
            + kingUnsafeCheck * popcount(unsafeChecks)
            + kingSliderBlocker * popcount(board.get_king_blockers(color))
            + 2 * flankAttacksCount / 8
            + info.mobility[!color].mg - info.mobility[color].mg
            - kingKnightDefender * popcount(ring & info.attackedSquares[Knight(color)])
            - kingBishopDefender * popcount(ring & info.attackedSquares[Bishop(color)])
            - 6 * pawnValue / 9;

    /*std::cout << "Flank Attacks: " << kingFlankAttack * flankAttacksCount << std::endl;
    std::cout << "Attackers: " << (info.kingAttackersNum[color] * info.kingAttackersWeight[color]) << std::endl;
    std::cout << "Ring Attacks: " << (kingRingAttackWeight * info.kingRingAttacks[color]) << std::endl;
    std::cout << "Ring Weak Square Attack: " << kingRingWeakSquareAttack * popcount(ring & weakSquares) << std::endl;
    std::cout << "Unsafe Checks: " << kingUnsafeCheck * popcount(unsafeChecks) << std::endl;
    std::cout << "Slider Blockers: " << kingSliderBlocker * popcount(board.get_king_blockers(color)) << std::endl;
    std::cout << "Flank Attacks Count: " << 2 * flankAttacksCount / 8 << std::endl;
    std::cout << "Mobility: " << (info.mobility[!color].mg - info.mobility[color].mg) << std::endl;
    std::cout << "Knight Defender: " << (kingKnightDefender * popcount(ring & info.attackedSquares[Knight(color)])) << std::endl;
    std::cout << "Bishop Defender: " << (kingBishopDefender * popcount(ring & info.attackedSquares[Bishop(color)])) << std::endl;
    std::cout << "Pawn Value: " << pawnValue << ":" << (6 * pawnValue / 9) << std::endl;
    std::cout << "Final: " << danger << ":" << std::max(0, (danger * danger / 2048)) << std::endl << std::endl;*/

    if (danger > 0) {
        value -= V(danger * danger / 2048, danger / 16);
    }

    if (!(board.pieces(Pawn(color)) & KING_FLANK[kingFile])) {
        value -= kingPawnlessFlank;
    }

    value -= kingFlankAttack * flankAttacksCount;

    value.mg += pawnValue;

    return V(std::min(80, value.mg), value.eg);

}

static const Value evaluate_passers(const Board& board, const Color color, const EvalInfo& info) {

    Value value;

    uint64_t passers = info.passedPawns & board.pieces(color);

    while (passers) {

        const unsigned int sq      = pop_lsb(passers);
        const unsigned int blocksq = sq + DIRECTIONS[color][UP];
        const int r                = relative_rank(color, sq);
        const unsigned int f       = file(sq);
        const unsigned int rfactor = (r - 2) * (r - 1) / 2;

        value += V(0, ((5 * kingDistance[info.kingSq[!color]][blocksq]) - (2 * kingDistance[info.kingSq[color]][blocksq])) * rfactor);

        if (r > 2 && !(SQUARES[blocksq] & board.pieces(ALLPIECES))) {

            Value bonus = V(0, 0);

            uint64_t path = FrontFileMask[color][sq];
            uint64_t behind = FrontFileMask[!color][sq];
            uint64_t attacked = PassedPawnMask[color][sq];

            uint64_t slidersBehind = behind & board.all_majors();

            if (!(slidersBehind & board.pieces(!color))) {
                attacked &= info.attackedSquares[!color];
            }

            if ((info.attackedSquares[color] & SQUARES[blocksq]) || (slidersBehind & board.pieces(color))) {
                bonus += passedPawnBlockSqDefended;
            }

            if (!attacked) {
                bonus += passedPawnNoAttacks;
            } else if (!(attacked & path)) {
                bonus += passedPawnSafePath;
            } else if (!(attacked & SQUARES[blocksq])) {
                bonus += passedPawnSafePush;
            }

            value += bonus * rfactor;

        }

        value += pawnPassedRankBonus[r] - pawnPassedFilePenalty[f];

    }

    return V(std::max(0, value.mg), std::max(0, value.eg));

}

static const Value evaluate_imbalances(const Board& board, const Color color) {

    Value value;

    const unsigned int pieceCounts[2][6] = {
        { board.piececount(WHITE_BISHOP) > 1, board.piececount(WHITE_PAWN), board.piececount(WHITE_KNIGHT), board.piececount(WHITE_BISHOP), board.piececount(WHITE_ROOK), board.piececount(WHITE_QUEEN) },
        { board.piececount(BLACK_BISHOP) > 1, board.piececount(BLACK_PAWN), board.piececount(BLACK_KNIGHT), board.piececount(BLACK_BISHOP), board.piececount(BLACK_ROOK), board.piececount(BLACK_QUEEN) }
    };

    for (unsigned int pt1index = 0; pt1index <= 5; pt1index++) {

        if (pieceCounts[color][pt1index] > 0) {

            int v = 0;

            for (unsigned int pt2index = 0; pt2index <= pt1index; pt2index++) {
                v += Imbalance[0][pt1index][pt2index] * pieceCounts[color][pt2index] + Imbalance[1][pt1index][pt2index] * pieceCounts[!color][pt2index];
            }

            value += V(v * pieceCounts[color][pt1index], v * pieceCounts[color][pt1index]);

        }

    }

    return value;

}

static const Value evaluate_threats(const Board& board, const Color color, const EvalInfo& info) {

    Value value;

    const uint64_t minorsAndMajors = board.pieces(!color) ^ board.pieces(Pawn(!color));
    const uint64_t strongSquares   = info.attackedSquares[Pawn(!color)] | (info.multiAttackedSquares[!color] & ~info.multiAttackedSquares[color]);
    const uint64_t defendedPieces  = minorsAndMajors & strongSquares;
    const uint64_t weakPieces      = board.pieces(!color) & ~strongSquares & info.attackedSquares[color];

    if (defendedPieces | weakPieces) {

        uint64_t minorAttacks = (defendedPieces | weakPieces) & (info.attackedSquares[Knight(color)] | info.attackedSquares[Bishop(color)]);
        while (minorAttacks) {

            const unsigned int sq = pop_lsb(minorAttacks);
            const PieceType pt = type(board.piecetype(sq));

            value += minorAttackWeight[pt_index(pt)];
            if (pt != PAWN) {
                value += pieceVulnerable * relative_rank(!color, sq);
            }

        }

        uint64_t rookAttacks = weakPieces & info.attackedSquares[Rook(color)];
        while (rookAttacks) {

            const unsigned int sq = pop_lsb(rookAttacks);
            const PieceType pt = type(board.piecetype(sq));

            value += rookAttackWeight[pt_index(pt)];
            if (pt != PAWN) {
                value += pieceVulnerable * relative_rank(!color, sq);
            }

        }

        value += KingAttackThreat * popcount(weakPieces & info.attackedSquares[King(color)]);

        value += HangingPiece * popcount(weakPieces & (~info.attackedSquares[!color] | (minorsAndMajors & info.multiAttackedSquares[color])));

    }

    value += mobilityRestriction * popcount(info.attackedSquares[!color] & ~strongSquares & info.attackedSquares[color]);

    const uint64_t safeSquares = info.attackedSquares[color] | ~info.attackedSquares[!color];
    const uint64_t safePawns   = board.pieces(PAWN, color) & safeSquares;

    value += safePawnAttack * popcount(generate_pawns_attacks(safePawns, color) & minorsAndMajors);

    uint64_t pawnPushes = shift_up(board.pieces(Pawn(color)), color) & ~board.pieces(ALLPIECES);
    pawnPushes |= shift_up(pawnPushes & PAWN_FIRST_PUSH_RANK[color], color) & ~board.pieces(ALLPIECES);
    pawnPushes &= ~info.attackedSquares[Pawn(!color)] & safeSquares;

    value += pawnPushThreat * popcount(generate_pawns_attacks(pawnPushes, color) & minorsAndMajors);

    // Evaluate possible safe attacks on queen
    uint64_t queens = board.pieces(QUEEN, !color);
    if (queens) {

        unsigned int sq = pop_lsb(queens);
        uint64_t knightAttackSquares = generateKnightMoves(sq, board.pieces(color)) & info.attackedSquares[Knight(color)];
        uint64_t bishopAttackSquares = generateBishopMoves(sq, board.pieces(ALLPIECES), 0) & info.attackedSquares[Bishop(color)];
        uint64_t rookAttackSquares   = generateRookMoves(sq, board.pieces(ALLPIECES), 0) & info.attackedSquares[Rook(color)];
        uint64_t safe = info.mobilityArea[color] & ~strongSquares;

        value += KnightQueenAttackThreat * popcount(knightAttackSquares & safe);

        safe &= info.multiAttackedSquares[color];

        value += BishopQueenAttackThreat * popcount(bishopAttackSquares & safe);
        value += RookQueenAttackThreat   * popcount(rookAttackSquares & safe);

    }

    return value;

}

static void init_eval_info(const Board& board, EvalInfo& info) {

    info.mobilityArea[WHITE] = ALL_SQUARES & ~((board.pieces(WHITE_KING) | board.pieces(WHITE_QUEEN)) | (board.pieces(WHITE_PAWN) & ((board.pieces(ALLPIECES) >> 8) | RANKS[6] | RANKS[5])) | info.attackedSquares[BLACK_PAWN]);
    info.mobilityArea[BLACK] = ALL_SQUARES & ~((board.pieces(BLACK_KING) | board.pieces(BLACK_QUEEN)) | (board.pieces(BLACK_PAWN) & ((board.pieces(ALLPIECES) << 8) | RANKS[1] | RANKS[2])) | info.attackedSquares[WHITE_PAWN]);

    info.kingSq[WHITE] = lsb_index(board.pieces(WHITE_KING));
    info.kingSq[BLACK] = lsb_index(board.pieces(BLACK_KING));

    info.kingRing[WHITE] = KingRing[WHITE][info.kingSq[WHITE]];
    info.kingRing[BLACK] = KingRing[BLACK][info.kingSq[BLACK]];

    info.attackedSquares[WHITE_KING] = AttackBitboards[WHITE_KING][info.kingSq[WHITE]];
    info.attackedSquares[BLACK_KING] = AttackBitboards[BLACK_KING][info.kingSq[BLACK]];

    info.attackedSquares[WHITE] |= info.attackedSquares[WHITE_KING] | info.attackedSquares[WHITE_PAWN];
    info.attackedSquares[BLACK] |= info.attackedSquares[BLACK_KING] | info.attackedSquares[BLACK_PAWN];

    info.multiAttackedSquares[WHITE] = info.attackedSquares[WHITE_KING] & info.attackedSquares[WHITE_PAWN];
    info.multiAttackedSquares[BLACK] = info.attackedSquares[BLACK_KING] & info.attackedSquares[BLACK_PAWN];

    info.kingAttackersNum[WHITE] = popcount(info.attackedSquares[WHITE_KING] & info.attackedSquares[BLACK_PAWN]);
    info.kingAttackersNum[BLACK] = popcount(info.attackedSquares[BLACK_KING] & info.attackedSquares[WHITE_PAWN]);

    info.blockedPawns[WHITE] = (board.pieces(WHITE_PAWN) << 8) & board.pieces(ALLPIECES);
    info.blockedPawns[BLACK] = (board.pieces(BLACK_PAWN) >> 8) & board.pieces(ALLPIECES);

}

const int evaluate(const Board& board) {

    Value value;

    EvalInfo info;

    unsigned int pieceCount;

    // Material draw
    if ((pieceCount = popcount(board.pieces(ALLPIECES))) <= 4) {
        if (board.checkMaterialDraw(pieceCount) == true)
            return 0;
    }

    PawnEntry * pentry = pawnTable.probe(board.pawnkey());
    if (pentry != NULL) {
        value += pentry->value;
        info.passedPawns = pentry->passedPawns;
        info.attackedSquares[WHITE_PAWN] = pentry->pawnWAttacks;
        info.attackedSquares[BLACK_PAWN] = pentry->pawnBAttacks;
        info.pawnAttacksSpan[WHITE] = pentry->pawnWAttacksSpan;
        info.pawnAttacksSpan[BLACK] = pentry->pawnBAttacksSpan;
    } else {
        info.attackedSquares[WHITE_PAWN] = board.gen_wpawns_attacks();
        info.attackedSquares[BLACK_PAWN] = board.gen_bpawns_attacks();
    }

    init_eval_info(board, info);

    // Material
    value += board.material(WHITE);
    value -= board.material(BLACK);

    // Piece square tables
    value += board.pst(WHITE);
    value -= board.pst(BLACK);

    // Pawns
    if (pentry == NULL) {
        Value pawnValue = evaluate_pawns(board, WHITE, info) - evaluate_pawns(board, BLACK, info);
        pawnTable.store(board.pawnkey(), pawnValue, info.attackedSquares[WHITE_PAWN], info.attackedSquares[BLACK_PAWN], info.passedPawns, info.pawnAttacksSpan[WHITE], info.pawnAttacksSpan[BLACK]);
        value += pawnValue;
    }

    // Pieces
    value += evaluate_knights(board, WHITE, info);
    value += evaluate_bishops(board, WHITE, info);
    value += evaluate_rooks(board, WHITE, info);
    value += evaluate_queens(board, WHITE, info);

    value -= evaluate_knights(board, BLACK, info);
    value -= evaluate_bishops(board, BLACK, info);
    value -= evaluate_rooks(board, BLACK, info);
    value -= evaluate_queens(board, BLACK, info);

    // Mobility
    value += info.mobility[WHITE];
    value -= info.mobility[BLACK];

    // King Safety
    value += evaluate_king_safety(board, WHITE, info);
    value -= evaluate_king_safety(board, BLACK, info);

    // Passed Pawns
    value += evaluate_passers(board, WHITE, info);
    value -= evaluate_passers(board, BLACK, info);

    // Threats
    value += evaluate_threats(board, WHITE, info);
    value -= evaluate_threats(board, BLACK, info);

    // Imbalances
    MaterialEntry * mentry = materialTable.probe(board.materialkey());
    if (mentry != NULL) {
        value += mentry->value;
    } else {
        Value imbalanceValue = evaluate_imbalances(board, WHITE) - evaluate_imbalances(board, BLACK);
        materialTable.store(board.materialkey(), imbalanceValue);
        value += imbalanceValue;
    }

    return ((board.turn() == WHITE) ? scaled_eval(board.scale(), value) : -scaled_eval(board.scale(), value)) + tempoBonus;

}

void evaluateInfo(const Board& board) {

    Value value;

    EvalInfo info;

    info.attackedSquares[WHITE_PAWN] = board.gen_wpawns_attacks();
    info.attackedSquares[BLACK_PAWN] = board.gen_bpawns_attacks();

    init_eval_info(board, info);

    // Material
    const Value whiteMaterialPsqt = board.material(WHITE) + board.pst(WHITE);
    const Value blackMaterialPsqt = board.material(BLACK) + board.pst(BLACK);

    // Pawns
    const Value whitePawns = evaluate_pawns(board, WHITE, info);
    const Value blackPawns = evaluate_pawns(board, BLACK, info);

    // Pieces
    const Value whiteKnights = evaluate_knights(board, WHITE, info);
    const Value whiteBishops = evaluate_bishops(board, WHITE, info);
    const Value whiteRooks   = evaluate_rooks(board, WHITE, info);
    const Value whiteQueens  = evaluate_queens(board, WHITE, info);

    const Value blackKnights = evaluate_knights(board, BLACK, info);
    const Value blackBishops = evaluate_bishops(board, BLACK, info);
    const Value blackRooks   = evaluate_rooks(board, BLACK, info);
    const Value blackQueens  = evaluate_queens(board, BLACK, info);

    // King Safety
    const Value whiteKingSafety = evaluate_king_safety(board, WHITE, info);
    const Value blackKingSafety = evaluate_king_safety(board, BLACK, info);

    // Passed Pawns
    const Value whitePassers = evaluate_passers(board, WHITE, info);
    const Value blackPassers = evaluate_passers(board, BLACK, info);

    // Threats
    const Value whiteThreats = evaluate_threats(board, WHITE, info);
    const Value blackThreats = evaluate_threats(board, BLACK, info);

    // Imbalances
    const Value whiteImbalances = evaluate_imbalances(board, WHITE);
    const Value blackImbalances = evaluate_imbalances(board, BLACK);

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

    value += whiteMaterialPsqt - blackMaterialPsqt;
    value += whiteImbalances - blackImbalances;
    value += whiteKnights - blackKnights;
    value += whiteBishops - blackBishops;
    value += whiteRooks - blackRooks;
    value += whiteQueens - blackQueens;
    value += whitePawns - blackPawns;
    value += info.mobility[WHITE] - info.mobility[BLACK];
    value += whitePassers - blackPassers;
    value += whiteKingSafety - blackKingSafety;
    value += whiteThreats - blackThreats;

    int finalValue = scaled_eval(board.scale(), value);
    int normalEval = evaluate(board);

    if (std::abs(finalValue + (board.turn() == WHITE ? tempoBonus : -tempoBonus)) != std::abs(normalEval)) {
        std::cout << "ERROR: Difference between evaluation functions!!!" << std::endl;
        std::cout << (finalValue + (board.turn() == WHITE ? tempoBonus : -tempoBonus)) << std::endl;
        std::cout << normalEval - tempoBonus << std::endl;
        abort();
    }

    std::cout << "Total(For White): " << scaled_eval(board.scale(), value) << std::endl;

}
