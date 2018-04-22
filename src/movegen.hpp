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

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.hpp"
#include "move.hpp"
#include "magic.hpp"
#include "bitboards.hpp"

// TODO: make size, index, scores private!

class MoveList {
    
    public:
        
        unsigned int size = 0;
        unsigned int index = 0;
        Move moves[250];
        int scores[250];
        
        inline void append(Move move) {
            moves[size++] = move;
        }
        
        void merge(MoveList list);
        unsigned int find(const Move move);
        void swap(const unsigned int index1, const unsigned int index2);
        const Move pick();
        void print();
    
};

static const uint64_t attackingPawns[2][64] = {
    
    {
        0x40000000000000, 0xa0000000000000, 0x50000000000000, 0x28000000000000, 0x14000000000000, 0xa000000000000, 0x5000000000000, 0x2000000000000,
        0x400000000000, 0xa00000000000, 0x500000000000, 0x280000000000, 0x140000000000, 0xa0000000000, 0x50000000000, 0x20000000000,
        0x4000000000, 0xa000000000, 0x5000000000, 0x2800000000, 0x1400000000, 0xa00000000, 0x500000000, 0x200000000,
        0x40000000, 0xa0000000, 0x50000000, 0x28000000, 0x14000000, 0xa000000, 0x5000000, 0x2000000,
        0x400000, 0xa00000, 0x500000, 0x280000, 0x140000, 0xa0000, 0x50000, 0x20000,
        0x4000, 0xa000, 0x5000, 0x2800, 0x1400, 0xa00, 0x500, 0x200,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    },
    {
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x40000000000000, 0xa0000000000000, 0x50000000000000, 0x28000000000000, 0x14000000000000, 0xa000000000000, 0x5000000000000, 0x2000000000000,
        0x400000000000, 0xa00000000000, 0x500000000000, 0x280000000000, 0x140000000000, 0xa0000000000, 0x50000000000, 0x20000000000,
        0x4000000000, 0xa000000000, 0x5000000000, 0x2800000000, 0x1400000000, 0xa00000000, 0x500000000, 0x200000000,
        0x40000000, 0xa0000000, 0x50000000, 0x28000000, 0x14000000, 0xa000000, 0x5000000, 0x2000000,
        0x400000, 0xa00000, 0x500000, 0x280000, 0x140000, 0xa0000, 0x50000, 0x20000,
        0x4000, 0xa000, 0x5000, 0x2800, 0x1400, 0xa00, 0x500, 0x200,
    }
    
};

static const uint64_t PawnAttacks[2][64] = {
    
    {
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x4000000000000000, 0xa000000000000000, 0x5000000000000000, 0x2800000000000000, 0x1400000000000000, 0xa00000000000000, 0x500000000000000, 0x200000000000000,
        0x40000000000000, 0xa0000000000000, 0x50000000000000, 0x28000000000000, 0x14000000000000, 0xa000000000000, 0x5000000000000, 0x2000000000000,
        0x400000000000, 0xa00000000000, 0x500000000000, 0x280000000000, 0x140000000000, 0xa0000000000, 0x50000000000, 0x20000000000,
        0x4000000000, 0xa000000000, 0x5000000000, 0x2800000000, 0x1400000000, 0xa00000000, 0x500000000, 0x200000000,
        0x40000000, 0xa0000000, 0x50000000, 0x28000000, 0x14000000, 0xa000000, 0x5000000, 0x2000000,
        0x400000, 0xa00000, 0x500000, 0x280000, 0x140000, 0xa0000, 0x50000, 0x20000,
        0x4000, 0xa000, 0x5000, 0x2800, 0x1400, 0xa00, 0x500, 0x200
    },
    {
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x400000000000, 0xa00000000000, 0x500000000000, 0x280000000000, 0x140000000000, 0xa0000000000, 0x50000000000, 0x20000000000,
        0x4000000000, 0xa000000000, 0x5000000000, 0x2800000000, 0x1400000000, 0xa00000000, 0x500000000, 0x200000000,
        0x40000000, 0xa0000000, 0x50000000, 0x28000000, 0x14000000, 0xa000000, 0x5000000, 0x2000000,
        0x400000, 0xa00000, 0x500000, 0x280000, 0x140000, 0xa0000, 0x50000, 0x20000,
        0x4000, 0xa000, 0x5000, 0x2800, 0x1400, 0xa00, 0x500, 0x200,
        0x40, 0xa0, 0x50, 0x28, 0x14, 0xa, 0x5, 0x2,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    }
    
};

// Bitboards white pawns
static const uint64_t pmovesPawnWhite[64] = {
    
    0, 0, 0, 0, 0, 0, 0, 0,
    9223372036854775808U, 4611686018427387904, 2305843009213693952, 1152921504606846976, 576460752303423488, 288230376151711744, 144115188075855872, 72057594037927936,
    36028797018963968, 18014398509481984, 9007199254740992, 4503599627370496, 2251799813685248, 1125899906842624, 562949953421312, 281474976710656,
    140737488355328, 70368744177664, 35184372088832, 17592186044416, 8796093022208, 4398046511104, 2199023255552, 1099511627776,
    549755813888, 274877906944, 137438953472, 68719476736, 34359738368, 17179869184, 8589934592, 4294967296,
    2147483648, 1073741824, 536870912, 268435456, 134217728, 67108864, 33554432, 16777216,
    2155872256, 1077936128, 538968064, 269484032, 134742016, 67371008, 33685504, 16842752,
    0, 0, 0, 0, 0, 0, 0, 0
    
};

// Bitboards black pawns
static const uint64_t pmovesPawnBlack[64] = {
    
    0, 0, 0, 0, 0, 0, 0, 0,
    141287244169216, 70643622084608, 35321811042304, 17660905521152, 8830452760576, 4415226380288, 2207613190144, 1103806595072,
    549755813888, 274877906944, 137438953472, 68719476736, 34359738368, 17179869184, 8589934592, 4294967296,
    2147483648, 1073741824, 536870912, 268435456, 134217728, 67108864, 33554432, 16777216,
    8388608, 4194304, 2097152, 1048576, 524288, 262144, 131072, 65536,
    32768, 16384, 8192, 4096, 2048, 1024, 512, 256,
    128, 64, 32, 16, 8, 4, 2, 1,
    0, 0, 0, 0, 0, 0, 0, 0
    
};

// Get all possible attackers for a given square
inline uint64_t getDelta(const unsigned int sqindex, const Side side, const uint64_t pawns, const uint64_t knights, const uint64_t bishops, const uint64_t rooks, const uint64_t queens, const uint64_t kings) {
    
    return (AttackBitboards[KNIGHT][sqindex] & knights) | (AttackBitboards[ROOK][sqindex] & (rooks | queens)) | (AttackBitboards[BISHOP][sqindex] & (bishops | queens)) | (AttackBitboards[KING][sqindex] & kings) | (attackingPawns[side][sqindex] & pawns);
    
}

// Check if a square is still on the board
inline bool sqIsValid(const int sqindex) {
    
    return (sqindex >= 0 && sqindex <= 63);
    
}

// Pawns
inline unsigned int getPawnPushSq(const unsigned int sqindex, const uint64_t allPieces, const int up) {        
    
    return (sqIsValid(sqindex + up) && (!(SQUARES[sqindex + up] & allPieces))) ? sqindex + up : NOSQ;
    
}

inline unsigned int getPawnDoublePushSq(const unsigned int sqindex, const uint64_t allPieces, const int up, const Side side) {
    
    return (sqIsValid(sqindex + (2 * up)) && (SQUARES[sqindex] & ((side == WHITE) ? RANK_7 : RANK_2)) && (!((SQUARES[sqindex + (2 * up)]) & allPieces)) && (!(SQUARES[sqindex + up] & allPieces))) ? sqindex + (2 * up) : NOSQ;
    
}

inline uint64_t generatePawnCaptures(const unsigned int sqindex, const uint64_t oppPieces, const Side side) {
    
    return PawnAttacks[side][sqindex] & oppPieces;
    
}

inline uint64_t generatePawnAttacks(const unsigned int sqindex, const Side side) {
    
    return PawnAttacks[side][sqindex];
    
}

// Pawns
inline uint64_t generatePawnMoves(const Side side, const unsigned int sq, const uint64_t allPieces, const uint64_t oppPieces) {
    
    return (SQUARES[getPawnPushSq(sq, allPieces, DIRECTIONS[side][UP])] | SQUARES[getPawnDoublePushSq(sq, allPieces, DIRECTIONS[side][UP], side)] | generatePawnCaptures(sq, oppPieces, side));
    
}

inline uint64_t generatePawnQuiets(const Side side, const unsigned int sq, const uint64_t allPieces) {

    return SQUARES[getPawnPushSq(sq, allPieces, DIRECTIONS[side][UP])] | SQUARES[getPawnDoublePushSq(sq, allPieces, DIRECTIONS[side][UP], side)];
    
}

inline uint64_t generate_pawns_attacks(const uint64_t pawns, const Side side) {
	
	return (side == WHITE) ? (((pawns & ~FILE_A) << 9) | ((pawns & ~FILE_H) << 7)) : (((pawns & ~FILE_A) >> 7) | ((pawns & ~FILE_H) >> 9));
	
}

// Knights & Kings
inline uint64_t generateKnightMoves(const unsigned int sqindex, const uint64_t ownPieces) {
    
    return AttackBitboards[KNIGHT][sqindex] & ~ownPieces;
    
}

inline uint64_t generateKingMoves(const unsigned int sqindex, const uint64_t ownPieces) {
    
    return AttackBitboards[KING][sqindex] & ~ownPieces;
    
}

// Sliders
inline uint64_t generateBishopMoves(const unsigned int sqindex, const uint64_t allPieces, const uint64_t ownPieces) {
    
    return ((magicMovesBishop[63 - sqindex][(((allPieces & occupancyMaskBishop[63 - sqindex]) * magicNumberBishop[63 - sqindex]) >> magicNumberShiftsBishop[63 - sqindex])]) & ~ownPieces);
    
}

inline uint64_t generateRookMoves(const unsigned int sqindex, const uint64_t allPieces, const uint64_t ownPieces) {
    
    return ((magicMovesRook[63 - sqindex][(((allPieces & occupancyMaskRook[63 - sqindex]) * magicNumberRook[63 - sqindex]) >> magicNumberShiftsRook[63 - sqindex])]) & ~ownPieces);
    
}

inline uint64_t generateQueenMoves(const unsigned int sqindex, const uint64_t allPieces, const uint64_t ownPieces) {
    
    return generateBishopMoves(sqindex, allPieces, ownPieces) | generateRookMoves(sqindex, allPieces, ownPieces);
    
}

extern MoveList gen_quiets(const Board& board, const Side side);
extern MoveList gen_caps(const Board& board, const Side side);
extern MoveList gen_all(const Board& board, const Side side);

#endif