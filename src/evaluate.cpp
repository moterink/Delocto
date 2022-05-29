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

#include "evaluate.hpp"
#include "movegen.hpp"
#include "uci.hpp"

EvalTerm PieceSquareTable[2][6][64];

// Piece Square Table Values
// The values are mirrored for each color at execution time
static const EvalTerm PstValues[6][32] = {

    // Pawns
    {

        V( 0,  0), V( 0,  0), V( 0,  0), V( 0,  0),
        V(-1, -7), V( 2, -2), V( 7,  4), V( 9,  5),
        V(-7, -3), V(-3, -4), V( 8, -2), V(11,  2),
        V(-5, -1), V(-5, -3), V( 6, -5), V(14, -4),
        V( 4,  4), V(-3,  4), V(-4, -1), V( 3, -6),
        V(-5, 10), V(-7,  6), V(-3,  7), V( 3, 14),
        V(-4,  2), V( 4, -2), V(-5,  8), V(-2, 11),
        V( 0,  0), V( 0,  0), V( 0,  0), V( 0,  0)

    },
    // Knights
    {

        V(-79, -49), V(-45, -35), V(-38, -22), V(-37, -8),
        V(-37, -33), V(-18, -26), V(-11,  -7), V( -4,  3),
        V(-30, -18), V( -9, -15), V(  2,  -2), V(  9, 13),
        V(-13, -17), V(  2,   0), V( 19,   6), V( 22, 16),
        V(-14, -19), V(  6,  -9), V( 20,   2), V( 24, 16),
        V( -5, -24), V( 13, -18), V( 30,  -8), V( 26,  9),
        V(-31, -30), V(-10, -21), V(  3, -17), V( 17,  8),
        V(-94, -46), V(-38, -42), V(-25, -25), V(-15, -8),

    },
    // Bishops
    {

        V(-21, -30), V(-2, -14), V(-5, -16), V(-13, -4),
        V( -8, -18), V( 3,  -6), V( 7,  -7), V(  1,  0),
        V( -4,  -8), V(11,   0), V(-1,  -3), V(  7,  6),
        V(  0, -12), V( 4,  -1), V(12,   0), V( 17,  8),
        V( -3, -11), V(14,  -3), V(11,  -5), V( 13,  8),
        V( -8, -12), V( 2,   1), V( 0,   0), V(  4,  8),
        V(-10, -16), V(-9,  -8), V( 5,  -3), V( -3,  4),
        V(-23, -24), V(-1, -19), V(-6, -18), V(-12, -9),

    },
    // Rooks
    {

        V(-11, -1), V( -6, -3), V(-3, -1), V( 1, -1),
        V( -8, -5), V( -5, -3), V(-2,  0), V( 4,  0),
        V(-10,  5), V( -3, -2), V( 1,  1), V( 0, -1),
        V( -6, -2), V( -2,  1), V(-2, -4), V(-3,  4),
        V(-11, -4), V( -6,  2), V( 0,  2), V( 3, -4),
        V(-11,  1), V( -2, -1), V( 2, -5), V( 5,  3),
        V( -4,  0), V(  3,  1), V( 5,  8), V( 6, -4),
        V(-10,  6), V(-11, -3), V(-3,  6), V( 2,  3)

    },
    // Queens
    {

        V( 1, -32), V(-2, -27), V(-2, -22), V( 2, -12),
        V(-1, -26), V( 2, -15), V( 4, -10), V( 6,  -2),
        V(-1, -18), V( 3,  -8), V( 6,  -4), V( 3,   1),
        V( 2, -11), V( 2,  -1), V( 4,   6), V( 4,  11),
        V( 0, -14), V( 7,  -3), V( 6,   4), V( 2,  10),
        V(-2, -18), V( 5,  -8), V( 3,  -6), V( 4,   0),
        V(-2, -23), V( 3, -13), V( 5, -11), V( 4,  -4),
        V(-1, -35), V(-1, -24), V( 0, -20), V(-1, -17)

    },
    // King
    {

        V(128,  0), V(153, 19), V(128, 38), V(89, 44),
        V(130, 27), V(143, 46), V(112, 65), V(86, 62),
        V( 93, 40), V(119, 65), V( 79, 77), V(56, 81),
        V( 79, 48), V( 90, 71), V( 64, 79), V(51, 79),
        V( 68, 46), V( 83, 78), V( 53, 92), V(32, 91),
        V( 56, 41), V( 75, 77), V( 40, 82), V(17, 89),
        V( 41, 19), V( 56, 46), V( 30, 60), V(12, 66),
        V( 30,  2), V( 41, 28), V( 23, 35), V( 0, 35)

    }

};

// Imbalance matrix
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

// Bitboards for outposts and boni for having minors on those squares or attacking them
static const Bitboard OutpostSquares[2]        = { BB_RANK_3 | BB_RANK_4 | BB_RANK_5, BB_RANK_6 | BB_RANK_5 | BB_RANK_4 };
static const EvalTerm OutpostBonus[2]          = { V(34, 11), V(17, 6) };
static const EvalTerm OutpostReachableBonus[2] = { V(17,  6), V( 8, 3) };

// Mobilty Values
// The more squares available to the piece, the higher the value
static const EvalTerm Mobility[4][28] = {

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
static const EvalTerm pawnDoubledPenalty           = V(8, 14);
static const EvalTerm pawnIsolatedPenalty[2]       = { V(11, 12), V(5, 7) };
static const EvalTerm pawnBackwardPenalty[2]       = { V(17, 11), V(10, 5) };
static const EvalTerm pawnLeverBonus[8]            = { V(0, 0), V(0, 0), V(0, 0), V(0, 0), V(7, 6), V(13, 13), V(0, 0), V(0, 0) };
static const int pawnConnectedBonus[8] = {

    0, 3, 4, 6, 14, 23, 40, 0

};
static const EvalTerm pawnPhalanxBonus[2][8] = {

    { V(0, 0), V(8, 0), V(9, 0), V(16, 2), V(37, 18), V(57, 43), V(105, 105), V(0, 0) },
    { V(0, 0), V(4, 0), V(4, 0), V( 9, 1), V(18,  9), V(29, 21), V( 52,  52), V(0, 0) }

};
static const EvalTerm pawnSupportBonus             = V(6, 2);
static const EvalTerm pawnPassedRankBonus[8]       = { V(0, 0), V(4, 13), V(8, 15), V(7, 19), V(29, 34), V(79, 83), V(130, 122), V(0, 0) };
static const EvalTerm pawnPassedFilePenalty[8]     = { V(0, 0), V(5, 4), V(10, 8), V(15, 12), V(15, 12), V(10, 8), V(5, 4), V(0, 0) };
static const EvalTerm pawnCandidateBonus[8]        = { V(0, 0), V(0, 0), V(5, 15),  V(10, 30), V(20, 45), V(30, 50),   V(40, 60),   V(0, 0) };

static const EvalTerm passedPawnNoAttacks          = V(16, 18);
static const EvalTerm passedPawnSafePath           = V( 9, 11);
static const EvalTerm passedPawnSafePush           = V( 4,  6);
static const EvalTerm passedPawnBlockSqDefended    = V( 2,  3);

// Bishops
static const EvalTerm bishopPawnsSameColorPenalty  = V( 1,  3);
static const EvalTerm bishopCenterAlignBonus       = V(21,  0);

// Minors
static const EvalTerm OutpostReachable[2]          = { V(8, 0), V(4, 0) };
static const EvalTerm minorPawnShield              = V(8, 1);

// Rooks
static const EvalTerm rookOpenFileBonus      = V(19,  9);
static const EvalTerm rookSemiOpenFileBonus  = V( 6,  4);
static const EvalTerm rookPawnAlignBonus     = V( 3, 12);
static const EvalTerm rookTrappedPenalty     = V(22,  2);

// Queens
static const EvalTerm UnsafeQueen = V(23, 7);

// King
static const unsigned queenSafeCheckWeight       = 365;
static const unsigned rookSafeCheckWeight        = 505;
static const unsigned bishopSafeCheckWeight      = 300;
static const unsigned knightSafeCheckWeight      = 370;
static const unsigned kingUnsafeCheck            =  65;
static const unsigned kingRingAttackWeight       =  32;
static const unsigned kingRingWeakSquareAttack   =  86;
static const unsigned kingSliderBlocker          =  65;
static const unsigned kingKnightDefender         =  47;
static const unsigned kingBishopDefender         =  18;
static const unsigned kingNoQueenAttacker        = 410;
static const unsigned attackerWeight[5]          = { 0, 36, 26, 21, 5 };
static const EvalTerm kingPawnlessFlank             = V(8, 45);
static const EvalTerm kingFlankAttack               = V(4,  0);
static const EvalTerm kingProtectorDistancePenalty  = V(3,  4);

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
static const EvalTerm safePawnAttack          = V(85, 46);
static const EvalTerm loosePawnWeight         = V(16, 28);
static const EvalTerm HangingPiece            = V(32, 17);
static const EvalTerm pawnPushThreat          = V(20, 12);
static const EvalTerm pieceVulnerable         = V( 6,  0);
static const EvalTerm mobilityRestriction     = V( 3,  3);
static const EvalTerm KnightQueenAttackThreat = V( 8,  6);
static const EvalTerm BishopQueenAttackThreat = V(28,  8);
static const EvalTerm RookQueenAttackThreat   = V(28,  8);
static const EvalTerm KingAttackThreat        = V(11, 42);
static const EvalTerm minorAttackWeight[PIECETYPE_COUNT] = {
    V(0, 15), V(19, 20), V(28, 22), V(34, 55), V(30, 59), V(0, 0)
};
static const EvalTerm rookAttackWeight[PIECETYPE_COUNT] = {
    V(0, 11), V(18, 34), V(16, 32), V( 0, 17), V(25, 19), V(0, 0)
};

// Tempo Bonus
static const int tempoBonus = 12;

int KingDistance[64][64];
static int kingPawnShelter[8][8];
static int kingPawnStorm[8][8];

// Initialize king distance arrays
void init_king_distance() {

    unsigned int findex, tindex;

    for (findex = 0; findex < 64; findex++) {
        for (tindex = 0; tindex < 64; tindex++) {
            KingDistance[findex][tindex] = std::max(std::abs((int)rank(tindex) - (int)rank(findex)), std::abs((int)file(tindex) - (int)file(findex)));
        }
    }

}

// Initialize the piece square tables
void init_psqt() {

    for (Piecetype pt = PAWN; pt < PIECE_NONE; pt++) {
        for (unsigned sq = 0; sq < 32; sq++) {
            const unsigned r = sq / 4;
            const unsigned f = sq & 0x3;
            EvalTerm value = PstValues[pt][sq];

            PieceSquareTable[WHITE][pt][8 * r + f]             = value;
            PieceSquareTable[WHITE][pt][8 * r + (7 - f)]       = value;
            PieceSquareTable[BLACK][pt][8 * (7 - r) + f]       = value;
            PieceSquareTable[BLACK][pt][8 * (7 - r) + (7 - f)] = value;

        }
    }

}

// Initialize evaluation terms
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

// Check if there is enough mating material on the board
bool Board::is_material_draw() const {

    if (bbPieces[PAWN] || bbPieces[ROOK] || bbPieces[QUEEN]) {
        return false;
    } else if (popcount(bbColors[BOTH]) > 3) {
        return false;
    }

    return true;

}

// Update attack info for calculating king safety
static void update_attack_info(Color color, Piecetype pt, Bitboard moves, EvalInfo& info) {

    // Merge the piece moves into the piece attacks bitboard
    info.pieceAttacks[color][pt] |= moves;
    info.multiAttacks[color] |= info.colorAttacks[color] & moves; // Squares which are attacked more than once
    info.colorAttacks[color] |= moves; // All attacks by a color on the board

    // Detect attacks on the king area
    const Bitboard kingAttacks = moves & info.kingRing[!color];

    // If there is an attack, count the attacked squares and add the attacker weight
    if (kingAttacks) {
        info.kingAttackersWeight[!color] += attackerWeight[pt];
        info.kingAttackersNum[!color]++;
        info.kingRingAttacks[!color] += popcount(kingAttacks);
    }

}

// Evaluate all knights on the board for a given color
static const EvalTerm evaluate_knights(const Board& board, const Color color, EvalInfo& info) {

    EvalTerm value;
    Bitboard knights = board.pieces(color, KNIGHT);
    while (knights) {

        Square sq = pop_lsb(knights);
        Bitboard moves = knight_target_squares(sq, 0);
        if (board.get_king_blockers(color) & SQUARES[sq]) {
            moves &= LineTable[sq][info.kingSq[color]];
        }

        // Bonus for outposts
        Bitboard outposts = OutpostSquares[color] & info.pieceAttacks[color][PAWN] & ~info.pawnAttacksSpan[!color];
        if (outposts & SQUARES[sq]) {
            value += OutpostBonus[0];
        } else if (outposts & moves & ~board.pieces(color)) {
            value += OutpostReachableBonus[0];
        }

        // Bonus for being behind a friendly pawn
        if (SQUARES[sq] & shift_down(board.pieces(PAWN), color)) {
            value += minorPawnShield;
        }

        // Penalty for being far away from the king
        value -= kingProtectorDistancePenalty * KingDistance[sq][info.kingSq[color]];

        info.mobility[color] += Mobility[0][popcount(moves & info.mobilityArea[color])];
        update_attack_info(color, KNIGHT, moves, info);

    }

    return value;

}

// Evaluate all bishops on the board for a given color
static const EvalTerm evaluate_bishops(const Board& board, const Color color, EvalInfo& info) {

    EvalTerm value;

    Bitboard bishops = board.pieces(color, BISHOP);

    while (bishops) {

        Square sq = pop_lsb(bishops);

        // Exclude queen for xrays
        Bitboard moves = bishop_target_squares(sq, board.pieces(BOTH) & ~board.pieces(QUEEN), 0);
        if (board.get_king_blockers(color) & SQUARES[sq]) {
            moves &= LineTable[sq][info.kingSq[color]];
        }

        // Bonus for outposts
        Bitboard outposts = OutpostSquares[color] & info.pieceAttacks[color][PAWN] & ~info.pawnAttacksSpan[!color];
        if (outposts & SQUARES[sq]) {
            value += OutpostBonus[1];
        } else if (outposts & moves & ~board.pieces(color)) {
            value += OutpostReachableBonus[1];
        }

        // Bonus for being behind a friendly pawn
        if (SQUARES[sq] & shift_down(board.pieces(PAWN), color)) {
            value += minorPawnShield;
        }

        // Penalty for having many pawns on the same square color as the bishop,
        // since this restricts the mobility of the bishop
        Bitboard pawnsOnSameColor = board.get_same_colored_squares(sq) & board.pieces(color, PAWN);
        value -= bishopPawnsSameColorPenalty * popcount(pawnsOnSameColor) * (1 + popcount(info.blockedPawns[color] & CENTRAL_FILES));

        // Bonus for being attacking central squares
        if (popcount(bishop_target_squares(sq, board.pieces(PAWN), 0) & CENTRAL_SQUARES) > 1) {
            value += bishopCenterAlignBonus;
        }

        // Penalty for being far away from the king
        value -= kingProtectorDistancePenalty * KingDistance[sq][info.kingSq[color]];

        info.mobility[color] += Mobility[1][popcount(moves & info.mobilityArea[color])];
        update_attack_info(color, BISHOP, moves, info);

    }

    return value;

}

// Evaluate all rooks on the board for a given color
static const EvalTerm evaluate_rooks(const Board& board, const Color color, EvalInfo& info) {

    EvalTerm value;

    Bitboard rooks = board.pieces(color, ROOK);
    while (rooks) {

        Square sq = pop_lsb(rooks);

        // Exclude queens and rooks for xrays
        Bitboard moves = rook_target_squares(sq, board.pieces(BOTH) & ~board.majors(), 0);
        if (board.get_king_blockers(color) & SQUARES[sq]) {
            moves &= LineTable[sq][info.kingSq[color]];
        }

        const File f = file(sq);
        const unsigned mob = popcount(moves & info.mobilityArea[color]);

        // Bonus for being on an open file (no pawns)
        if (!(FILES[f] & board.pieces(PAWN))) {
            value += rookOpenFileBonus;
        // Bonus for being on a semi-open file (no friendly pawns)
        } else if (!(FILES[f] & board.pieces(color, PAWN))) {
            value += rookSemiOpenFileBonus;
        } else {
            // Penalty for being in the corner and having low mobility
            const int kingFile = file(info.kingSq[color]);
            if (mob <= 3 && ((kingFile > 3) == (f > kingFile))) {
                value -= rookTrappedPenalty;
            }

        }

        // Bonus for aligning on file with enemy pawn
        if (relative_rank(color, sq) >= 4) {
            value += rookPawnAlignBonus * popcount(moves & board.pieces(!color, PAWN));
        }

        info.mobility[color] += Mobility[2][mob];
        update_attack_info(color, ROOK, moves, info);

    }

    return value;

}

// Evaluate all queens on the board for a given color
static const EvalTerm evaluate_queens(const Board& board, const Color color, EvalInfo& info) {

    EvalTerm value;

    Bitboard queens = board.pieces(color, QUEEN);
    while (queens) {

        Square sq = pop_lsb(queens);
        Bitboard moves = queen_target_squares(sq, board.pieces(BOTH), 0);
        if (board.get_king_blockers(color) & SQUARES[sq]) {
            moves &= LineTable[sq][info.kingSq[color]];
        }

        // Penalty for being in a discoverd attack by a slider
        if (board.get_slider_blockers(board.pieces(!color, BISHOP) | board.pieces(!color, ROOK), sq)) {
            value -= UnsafeQueen;
        }

        info.mobility[color] += Mobility[3][popcount(moves & info.mobilityArea[color])];
        update_attack_info(color, QUEEN, moves, info);

    }

    return value;

}

// Evaluate all pawns on the board for a given color
static const EvalTerm evaluate_pawns(const Board& board, const Color color, EvalInfo& info) {

    EvalTerm value;

    const Bitboard ownPawns = board.pieces(color, PAWN);
    const Bitboard oppPawns = board.pieces(!color, PAWN);

    Bitboard pawns = ownPawns;

    while (pawns) {

        const Square sq = pop_lsb(pawns);
        const File f    = file(sq);
        const Rank r    = relative_rank(color, sq);

        const Bitboard front      = FrontFileMask[color][sq];
        const Bitboard neighbours = ADJ_FILES[f] & ownPawns;
        const Bitboard stoppers   = PassedPawnMask[color][sq] & oppPawns;
        const Bitboard lever      = PawnAttacks[color][sq] & oppPawns;

        info.pawnAttacksSpan[color] |= PawnAttacksSpan[color][sq];

        const bool doubled       = front & ownPawns; // Two friendly pawns on the same file
        const bool opposed       = front & oppPawns;
        const bool isolated      = !neighbours; // A pawn is isolated if there are no friendly pawns on the adjacent squares
        const bool passed        = !(stoppers ^ lever); // No enemy pawn can hinder the pawn from promoting
        const Bitboard supported = (neighbours & RANKS[rank(sq + direction(color, DOWN))]);
        const Bitboard phalanx   = (neighbours & RANKS[rank(sq)]); // Friendly pawns which are on the same rank and on adjacent squares

        bool backward = false;

        // A pawn is considered backwards if there are no other friendly pawns
        // on the same rank or behind the pawn and there are also no enemy pawns
        // ahead of the pawn. The pawn is only backwards however if it can be captured
        // by enemy pawns if he pushes one square north.
        if (!isolated && !phalanx && r <= 4 && !lever) {
            const Bitboard br = RANKS[rank(lsb_index(most_backward(color, neighbours | stoppers)))];
            backward = (br | shift_up(ADJ_FILES[f] & br, color)) & stoppers;
        }

        // Penalty for doubled pawns
        if (doubled) {
            value -= pawnDoubledPenalty;
        // Passed pawns bitboard gets saved in a pawn hash table entry
        } else if (passed) {
            info.passedPawns |= SQUARES[sq];
        }

        if (phalanx || supported) {
            int bonus = pawnConnectedBonus[r] * (phalanx ? 3 : 2) / (opposed ? 2 : 1) + 8 * popcount(supported);
            value += V(bonus, bonus * (r - 2) / 4);
        }

        // Bonus for adjacent friendy pawns on the same rank
        if (phalanx) {
            value += pawnPhalanxBonus[opposed][r];
        // Penalty for not having any friendly pawns on adjacent files
        } else if (isolated) {
            value -= pawnIsolatedPenalty[opposed];
        // Penalty for not having
        } else if (backward) {
            value -= pawnBackwardPenalty[opposed];
        }

        // Bonus for attacking enemy pawns
        if (lever) {
            value += pawnLeverBonus[r];
        }

    }

    return value;

}

// Evaluate Pawn Shelter and Pawn Storm on the board for a given color
static int evaluate_shelter_storm(const Board& board, const Color color, const unsigned kingSq, const int oldValue) {

    int value = 0;

    const Bitboard notBehind = ~KingShelterSpan[!color][kingSq];
    const Bitboard ourPawns  = board.pieces(color, PAWN) & notBehind;
    const Bitboard oppPawns  = board.pieces(!color, PAWN) & notBehind;
    const int kingFile       = file(kingSq);
    const int centralFile    = std::max(1, std::min(kingFile, 6)); // The central file for the shelter. Adjusted on the edges

    // Loop over each file ahead of the king
    for (int f = centralFile - 1; f <= centralFile + 1; f++) {

        Bitboard owns = FILES[f] & ourPawns;
        Bitboard opps = FILES[f] & oppPawns;

        // Get the square of the most backward friendly and most forward enemy pawn on the file
        unsigned ownRank = owns ? relative_rank(color, lsb_index(most_backward(color, owns))) : 0;
        unsigned oppRank = opps ? relative_rank(color, lsb_index(most_forward(!color, opps))) : 0;

        // Add shelter bonus for our pawns and subtract storm penalty for enemy pawns
        value += kingPawnShelter[f][ownRank];
        value -= kingPawnStorm[f][oppRank];

    }

    // If we have not castled yet and the shelter score is greater
    // once we castle, use the better value
    if (oldValue > value) {
        return oldValue;
    }

    return value;

}

// King Safety Evaluation
static const EvalTerm evaluate_king_safety(const Board& board, const Color color, const EvalInfo& info) {

    EvalTerm value = V(0, 0);
    // Evaluate pawn shelter and pawn storms
    int pawnValue = evaluate_shelter_storm(board, color, info.kingSq[color], -VALUE_INFINITE);

    // Idea from Stockfish
    // If we can still castle, and the shelter bonus after the castling is greater, use the better value
    if (board.may_castle(CASTLE_TYPES[color][CASTLE_SHORT])) {
        pawnValue = evaluate_shelter_storm(board, color, CASTLE_KING_TARGET_SQUARE[color][CASTLE_SHORT], pawnValue);
    }
    if (board.may_castle(CASTLE_TYPES[color][CASTLE_LONG])) {
        pawnValue = evaluate_shelter_storm(board, color, CASTLE_KING_TARGET_SQUARE[color][CASTLE_LONG], pawnValue);
    }

    const File kingFile = file(info.kingSq[color]);

    // All squares attacked on the flank of the king excluding the base ranks (first 3 ranks) for our color
    // Count the number of squares which are attacked once and multiple times
    const Bitboard flankAttackedSquares = KING_FLANK[kingFile] & (SQUARES_ALL ^ COLOR_BASE_RANKS[!color]) & info.colorAttacks[!color];
    const unsigned flankAttacksCount = popcount(flankAttackedSquares) + popcount(flankAttackedSquares & info.multiAttacks[!color]);

    // The squares surrounding the king
    const Bitboard ring = KingRing[color][info.kingSq[color]];

    // Weak squares are squares are squares which we do not attack or only with queens or the king.
    // These squares are considered weak if the enemy attacks them, but not if we protect them with
    // multiple pieces
    const Bitboard weakSquares = (info.colorAttacks[!color] & ~info.multiAttacks[color]) & (~info.colorAttacks[color] | info.pieceAttacks[color][QUEEN] | info.pieceAttacks[color][KING]);

    // A square is considered safe for a enemy check if there it is not attacked by us or
    // it is a weak square which is attacked by multiple enemy pieces
    const Bitboard safeCheckSquares = ~board.pieces(!color) & (~info.colorAttacks[color] | (weakSquares & info.multiAttacks[!color]));

    const Bitboard knightCheckSquares = knight_target_squares(info.kingSq[color], board.pieces(color));
    const Bitboard bishopCheckSquares = bishop_target_squares(info.kingSq[color], board.pieces(BOTH) ^ board.pieces(color, QUEEN), 0);
    const Bitboard rookCheckSquares   = rook_target_squares(info.kingSq[color], board.pieces(BOTH) ^ board.pieces(color, QUEEN), 0);

    Bitboard unsafeChecks = 0;
    const Bitboard queenChecks  = info.pieceAttacks[!color][QUEEN] & (bishopCheckSquares | rookCheckSquares)  & ~info.pieceAttacks[color][QUEEN];
    const Bitboard rookChecks   = info.pieceAttacks[!color][ROOK] & rookCheckSquares;
    const Bitboard bishopChecks = info.pieceAttacks[!color][BISHOP] & bishopCheckSquares;
    const Bitboard knightChecks = info.pieceAttacks[!color][KNIGHT] & knightCheckSquares;

    int danger = 0;

    // If there is no enemy queen, decrease the king danger value drastically
    if (!board.pieces(!color, QUEEN)) {
        danger -= kingNoQueenAttacker;
    }

    // Add a check weight value to the king danger balance if there is a safe check
    if (queenChecks & (safeCheckSquares & ~rookChecks)) {
        danger += queenSafeCheckWeight;
    }

    if (rookChecks & safeCheckSquares) {
        danger += rookSafeCheckWeight;
    } else {
        unsafeChecks |= rookChecks;
    }

    if (bishopChecks & (safeCheckSquares & ~queenChecks)) {
        danger += bishopSafeCheckWeight;
    } else {
        unsafeChecks |= bishopChecks;
    }

    if (knightChecks & safeCheckSquares) {
        danger += knightSafeCheckWeight;
    } else {
        unsafeChecks |= knightChecks;
    }

    unsafeChecks &= info.mobilityArea[!color];

    /*std::cout << "Color: " << color << std::endl;
    std::cout << "Safe Checks & No Queen: " << danger << std::endl;*/

    // Calculate the king danger.
    // We multiply the total king attacker weights by the number of total attacks
    // Attacks on the king ring and on weak squares are also considered.
    // We count the total number of unsafe checks and add it to the danger value.
    // Also, minors within the king ring count as defenders and decrease the king danger.
    danger += info.kingAttackersNum[color] * info.kingAttackersWeight[color]
            + kingRingAttackWeight * info.kingRingAttacks[color]
            + kingRingWeakSquareAttack * popcount(ring & weakSquares)
            + kingUnsafeCheck * popcount(unsafeChecks)
            + kingSliderBlocker * popcount(board.get_king_blockers(color))
            + 2 * flankAttacksCount / 8
            + info.mobility[!color].mg - info.mobility[color].mg
            - kingKnightDefender * popcount(ring & info.pieceAttacks[color][KNIGHT])
            - kingBishopDefender * popcount(ring & info.pieceAttacks[color][BISHOP])
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
    std::cout << "Pawn EvalTerm: " << pawnValue << ":" << (6 * pawnValue / 9) << std::endl;
    std::cout << "Final: " << danger << ":" << std::max(0, (danger * danger / 2048)) << std::endl << std::endl;*/

    // We transform the king danger into a value which is subtracted from the king safety
    if (danger > 0) {
        value -= V(danger * danger / 2048, danger / 16);
    }

    // If there are no pawns on the king flank, we decrease the king safety value
    if (!(board.pieces(color, PAWN) & KING_FLANK[kingFile])) {
        value -= kingPawnlessFlank;
    }

    value -= kingFlankAttack * flankAttacksCount;

    value.mg += pawnValue;

    return V(std::min(80, value.mg), value.eg);

}

// Evaluate Passed Pawns on the board by a given color
static const EvalTerm evaluate_passers(const Board& board, const Color color, const EvalInfo& info) {

    EvalTerm value;

    Bitboard passers = info.passedPawns & board.pieces(color);

    // Loop over all passed pawns
    while (passers) {

        const Square sq        = pop_lsb(passers);
        const Square blocksq   = sq + direction(color, UP);
        const Rank r           = relative_rank(color, sq);
        const File f           = file(sq);
        const unsigned rfactor = (r - 2) * (r - 1) / 2;

        // Add a bonus based on both king's distance to the pawn
        value += V(0, ((5 * KingDistance[info.kingSq[!color]][blocksq]) - (2 * KingDistance[info.kingSq[color]][blocksq])) * rfactor);

        if (r > 2 && !(SQUARES[blocksq] & board.pieces(BOTH))) {

            EvalTerm bonus = V(0, 0);

            Bitboard path     = FrontFileMask[color][sq];
            Bitboard behind   = FrontFileMask[!color][sq];
            Bitboard attacked = PassedPawnMask[color][sq];

            Bitboard majorsBehind = behind & board.majors();

            // If there are no major enemy pieces on the file behind the pawn,
            // check if there are any squares in front of the pawn atttacked
            if (!(majorsBehind & board.pieces(!color))) {
                attacked &= info.colorAttacks[!color];
            }

            // If the block square is defended, add a bonus
            if ((info.colorAttacks[color] & SQUARES[blocksq]) || (majorsBehind & board.pieces(color))) {
                bonus += passedPawnBlockSqDefended;
            }

            // Add a bonus based on the kind of attacks on the pawn
            if (!attacked) {
                bonus += passedPawnNoAttacks;
            } else if (!(attacked & path)) {
                bonus += passedPawnSafePath;
            } else if (!(attacked & SQUARES[blocksq])) {
                bonus += passedPawnSafePush;
            }

            // Multiply the bonus with a factor based on the relative rank
            // and add it to the passed pawns evaluation
            value += bonus * rfactor;

        }

        // Add a bonus based on the rank and the file of the pawn
        value += pawnPassedRankBonus[r] - pawnPassedFilePenalty[f];

    }

    // No negative evaluation for passed pawns
    return V(std::max(0, value.mg), std::max(0, value.eg));

}

// Evaluate all material imbalances on the board for the given color
static const EvalTerm evaluate_imbalances(const Board& board, const Color color) {

    EvalTerm value;

    const unsigned pieceCounts[2][6] = {
        { board.piececount(WHITE, BISHOP) > 1, board.piececount(WHITE, PAWN), board.piececount(WHITE, KNIGHT), board.piececount(WHITE, BISHOP), board.piececount(WHITE, ROOK), board.piececount(WHITE, QUEEN) },
        { board.piececount(BLACK, BISHOP) > 1, board.piececount(BLACK, PAWN), board.piececount(BLACK, KNIGHT), board.piececount(BLACK, BISHOP), board.piececount(BLACK, ROOK), board.piececount(BLACK, QUEEN) }
    };

    // Loop over all piece types
    for (unsigned pt1index = 0; pt1index <= 5; pt1index++) {

        if (pieceCounts[color][pt1index] > 0) {

            int v = 0;

            // Loop over all the piece types again, and add the imbalance value based
            // on how many pieces are on the board
            for (unsigned pt2index = 0; pt2index <= pt1index; pt2index++) {
                v += Imbalance[0][pt1index][pt2index] * pieceCounts[color][pt2index] + Imbalance[1][pt1index][pt2index] * pieceCounts[!color][pt2index];
            }

            // Multiply the value with the number of pieces of the current type
            value += V(v * pieceCounts[color][pt1index], v * pieceCounts[color][pt1index]);

        }

    }

    return value;

}

// Evaluate all threats on the board for the given color
static const EvalTerm evaluate_threats(const Board& board, const Color color, const EvalInfo& info) {

    EvalTerm value;

    const Bitboard nonPawnPieces  = board.pieces(!color) ^ board.pieces(!color, PAWN); //board.minors_and_majors(!color);
    const Bitboard strongSquares  = info.pieceAttacks[!color][PAWN] | (info.multiAttacks[!color] & ~info.multiAttacks[color]);
    const Bitboard defendedPieces = nonPawnPieces & strongSquares;
    const Bitboard weakPieces     = board.pieces(!color) & ~strongSquares & info.colorAttacks[color];

    // Evaluate minors and rooks attacking weak and defended pieces
    // Add a bigger bonus for pieces which are are close to our base
    // since they are more vulnerable there
    if (defendedPieces | weakPieces) {

        Bitboard minorAttacks = (defendedPieces | weakPieces) & (info.pieceAttacks[color][KNIGHT] | info.pieceAttacks[color][BISHOP]);
        while (minorAttacks) {

            const Square sq = pop_lsb(minorAttacks);
            const Piecetype pt = board.piecetype(sq);

            value += minorAttackWeight[pt];
            if (pt != PAWN) {
                value += pieceVulnerable * relative_rank(!color, sq);
            }

        }

        Bitboard rookAttacks = weakPieces & info.pieceAttacks[color][ROOK];
        while (rookAttacks) {

            const Square sq = pop_lsb(rookAttacks);
            const Piecetype pt = board.piecetype(sq);

            value += rookAttackWeight[pt];
            if (pt != PAWN) {
                value += pieceVulnerable * relative_rank(!color, sq);
            }

        }

        // Bonus for king attacking weak pieces
        value += KingAttackThreat * popcount(weakPieces & info.pieceAttacks[color][KING]);

        // Bonus for unprotected weak pieces and weak pieces which are attacked multiple times by us
        value += HangingPiece * popcount(weakPieces & (~info.colorAttacks[!color] | (nonPawnPieces & info.multiAttacks[color])));

    }

    // Bonus for restricting the mobility of enemy pieces
    value += mobilityRestriction * popcount(info.colorAttacks[!color] & ~strongSquares & info.colorAttacks[color]);

    const Bitboard safeSquares = info.colorAttacks[color] | ~info.colorAttacks[!color];
    const Bitboard safePawns   = board.pieces(color, PAWN) & safeSquares;

    // Bonus for safe pawns attacking minor and major pieces
    value += safePawnAttack * popcount(generate_pawns_attacks(safePawns, color) & nonPawnPieces);

    // Bonus for pawn pushes which attack enemy pieces
    Bitboard pawnPushes = shift_up(board.pieces(color, PAWN), color) & ~board.pieces(BOTH);
    pawnPushes |= shift_up(pawnPushes & PAWN_FIRST_PUSH_RANK[color], color) & ~board.pieces(BOTH);
    pawnPushes &= ~info.pieceAttacks[!color][PAWN] & safeSquares;

    value += pawnPushThreat * popcount(generate_pawns_attacks(pawnPushes, color) & nonPawnPieces);

    // Evaluate possible safe attacks on queen
    Bitboard queens = board.pieces(!color, QUEEN);
    if (queens) {

        Square sq = pop_lsb(queens);
        Bitboard knightAttackSquares = knight_target_squares(sq, board.pieces(color)) & info.pieceAttacks[color][KNIGHT];
        Bitboard bishopAttackSquares = bishop_target_squares(sq, board.pieces(BOTH), 0) & info.pieceAttacks[color][BISHOP];
        Bitboard rookAttackSquares   = rook_target_squares(sq, board.pieces(BOTH), 0) & info.pieceAttacks[color][ROOK];
        Bitboard safe = info.mobilityArea[color] & ~strongSquares;

        value += KnightQueenAttackThreat * popcount(knightAttackSquares & safe);

        safe &= info.multiAttacks[color];

        value += BishopQueenAttackThreat * popcount(bishopAttackSquares & safe);
        value += RookQueenAttackThreat   * popcount(rookAttackSquares & safe);

    }

    return value;

}

// Initialize the evaluation info object
static void init_eval_info(const Board& board, EvalInfo& info) {

    // The mobility area is all squares we count for mobility.
    // We count all squares except for those attacked by enemy pawns
    // and blocked friendly pawns, our queens and king.
    info.mobilityArea[WHITE] = SQUARES_ALL & ~((board.pieces(WHITE, KING) | board.pieces(WHITE, QUEEN)) | (board.pieces(WHITE, PAWN) & (shift_down(board.pieces(BOTH), WHITE) | BB_RANK_2 | BB_RANK_3)) | info.pieceAttacks[BLACK][PAWN]);
    info.mobilityArea[BLACK] = SQUARES_ALL & ~((board.pieces(BLACK, KING) | board.pieces(BLACK, QUEEN)) | (board.pieces(BLACK, PAWN) & (shift_down(board.pieces(BOTH), BLACK) | BB_RANK_7 | BB_RANK_6)) | info.pieceAttacks[WHITE][PAWN]);

    // Set king square and ring area bitboards for each color
    info.kingSq[WHITE] = lsb_index(board.pieces(WHITE, KING));
    info.kingSq[BLACK] = lsb_index(board.pieces(BLACK, KING));

    info.kingRing[WHITE] = KingRing[WHITE][info.kingSq[WHITE]];
    info.kingRing[BLACK] = KingRing[BLACK][info.kingSq[BLACK]];

    // Bitboards for cumulative piece attacks for each piece type for each color
    info.pieceAttacks[WHITE][KING] = KingAttacks[info.kingSq[WHITE]];
    info.pieceAttacks[BLACK][KING] = KingAttacks[info.kingSq[BLACK]];

    // Bitboards for attacks of each color
    info.colorAttacks[WHITE] |= info.pieceAttacks[WHITE][KING] | info.pieceAttacks[WHITE][PAWN];
    info.colorAttacks[BLACK] |= info.pieceAttacks[BLACK][KING] | info.pieceAttacks[BLACK][PAWN];

    // Squares which are attacked by multiple pieces
    info.multiAttacks[WHITE] = info.pieceAttacks[WHITE][KING] & info.pieceAttacks[WHITE][PAWN];
    info.multiAttacks[BLACK] = info.pieceAttacks[BLACK][KING] & info.pieceAttacks[BLACK][PAWN];

    // Count of total pieces attacking the king for each color
    info.kingAttackersNum[WHITE] = popcount(info.pieceAttacks[WHITE][KING] & info.pieceAttacks[BLACK][PAWN]);
    info.kingAttackersNum[BLACK] = popcount(info.pieceAttacks[BLACK][KING] & info.pieceAttacks[WHITE][PAWN]);

    // Bitboards of pawns which cannot advance because there is a piece on their push square
    info.blockedPawns[WHITE] = shift_up(board.pieces(WHITE, PAWN), WHITE) & board.pieces(BOTH);
    info.blockedPawns[BLACK] = shift_up(board.pieces(BLACK, PAWN), BLACK) & board.pieces(BOTH);

}

// Evaluate the position statically
int evaluate(const Board& board, const unsigned threadIndex) {

    Thread* thread = Threads.get_thread(threadIndex);

    EvalTerm value;

    EvalInfo info;

    // Check for draw by insufficient material
    if (board.is_material_draw()) {
        return 0;
    }

    // Probe the pawn hash table
    PawnEntry * pentry = thread->pawnTable.probe(board.pawnkey());
    if (pentry != NULL) {
        value += pentry->value;
        info.passedPawns = pentry->passedPawns;
        info.pieceAttacks[WHITE][PAWN] = pentry->pawnWAttacks;
        info.pieceAttacks[BLACK][PAWN] = pentry->pawnBAttacks;
        info.pawnAttacksSpan[WHITE] = pentry->pawnWAttacksSpan;
        info.pawnAttacksSpan[BLACK] = pentry->pawnBAttacksSpan;
        assert(pentry->value == (evaluate_pawns(board, WHITE, info) - evaluate_pawns(board, BLACK, info)));
    } else {
        // If there is no entry available, compute the pawn attacks
        info.pieceAttacks[WHITE][PAWN] = board.gen_white_pawns_attacks();
        info.pieceAttacks[BLACK][PAWN] = board.gen_black_pawns_attacks();
    }

    // Initialize the evaluation
    init_eval_info(board, info);

    // Material balance
    value += board.material(WHITE);
    value -= board.material(BLACK);

    // Piece square table values
    value += board.pst(WHITE);
    value -= board.pst(BLACK);

    // Pawns Evaluation (skip if we already have a value from the hash table)
    if (pentry == NULL) {
        EvalTerm pawnValue = evaluate_pawns(board, WHITE, info) - evaluate_pawns(board, BLACK, info);
        thread->pawnTable.store(board.pawnkey(), pawnValue, info.pieceAttacks[WHITE][PAWN], info.pieceAttacks[BLACK][PAWN], info.passedPawns, info.pawnAttacksSpan[WHITE], info.pawnAttacksSpan[BLACK]);
        value += pawnValue;
    }

    // Evaluate the pieces
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
    MaterialEntry * mentry = thread->materialTable.probe(board.materialkey());
    if (mentry != NULL) {
        value += mentry->value;
        assert(mentry->value == (evaluate_imbalances(board, WHITE) - evaluate_imbalances(board, BLACK)));
    } else {
        EvalTerm imbalanceValue = evaluate_imbalances(board, WHITE) - evaluate_imbalances(board, BLACK);
        thread->materialTable.store(board.materialkey(), imbalanceValue);
        value += imbalanceValue;
    }

    assert(std::abs(scaled_eval(board.scale(), value)) < VALUE_MATE_MAX);

    // Return the scaled evaluation and add a tempo bonus for the color to move
    // Also, invert the evaluation to match the perspective of the color to move
    return ((board.turn() == WHITE) ?  scaled_eval(board.scale(), value)
                                    : -scaled_eval(board.scale(), value)) + tempoBonus;

}

// Evaluate the position statically and print a table containing
// all evaluation terms to the console. Useful for debugging
void evaluate_info(const Board& board) {

    EvalTerm value;

    EvalInfo info;

    info.pieceAttacks[WHITE][PAWN] = board.gen_white_pawns_attacks();
    info.pieceAttacks[BLACK][PAWN] = board.gen_black_pawns_attacks();

    init_eval_info(board, info);

    // Material
    const EvalTerm whiteMaterialPsqt = board.material(WHITE) + board.pst(WHITE);
    const EvalTerm blackMaterialPsqt = board.material(BLACK) + board.pst(BLACK);

    // Pawns
    const EvalTerm whitePawns = evaluate_pawns(board, WHITE, info);
    const EvalTerm blackPawns = evaluate_pawns(board, BLACK, info);

    // Pieces
    const EvalTerm whiteKnights = evaluate_knights(board, WHITE, info);
    const EvalTerm whiteBishops = evaluate_bishops(board, WHITE, info);
    const EvalTerm whiteRooks   = evaluate_rooks(board, WHITE, info);
    const EvalTerm whiteQueens  = evaluate_queens(board, WHITE, info);

    const EvalTerm blackKnights = evaluate_knights(board, BLACK, info);
    const EvalTerm blackBishops = evaluate_bishops(board, BLACK, info);
    const EvalTerm blackRooks   = evaluate_rooks(board, BLACK, info);
    const EvalTerm blackQueens  = evaluate_queens(board, BLACK, info);

    // King Safety
    const EvalTerm whiteKingSafety = evaluate_king_safety(board, WHITE, info);
    const EvalTerm blackKingSafety = evaluate_king_safety(board, BLACK, info);

    // Passed Pawns
    const EvalTerm whitePassers = evaluate_passers(board, WHITE, info);
    const EvalTerm blackPassers = evaluate_passers(board, BLACK, info);

    // Threats
    const EvalTerm whiteThreats = evaluate_threats(board, WHITE, info);
    const EvalTerm blackThreats = evaluate_threats(board, BLACK, info);

    // Imbalances
    const EvalTerm whiteImbalances = evaluate_imbalances(board, WHITE);
    const EvalTerm blackImbalances = evaluate_imbalances(board, BLACK);

    // Output evaluation terms to the console
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
    int normalEval = evaluate(board, 0);

    if (std::abs(finalValue + (board.turn() == WHITE ? tempoBonus : -tempoBonus)) != std::abs(normalEval)) {
        std::cout << "ERROR: Difference between evaluation functions!!!" << std::endl;
        std::cout << (finalValue + (board.turn() == WHITE ? tempoBonus : -tempoBonus)) << std::endl;
        std::cout << normalEval - tempoBonus << std::endl;
        abort();
    }

    std::cout << "Total(For White): " << scaled_eval(board.scale(), value) << std::endl;

}
