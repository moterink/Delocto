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

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.hpp"
#include "move.hpp"
#include "magic.hpp"
#include "bitboards.hpp"

// TODO: make size, index, values private!

class MoveList {

    public:

        unsigned int size = 0;
        unsigned int index = 0;
        Move moves[250];
        int values[250];

        inline void append(Move move) {
            moves[size++] = move;
        }

        void merge(MoveList list);
        unsigned int find(const Move move);
        void swap(const unsigned int index1, const unsigned int index2);
        const Move pick();
        void print();

};

// Check if a square is still on the board
inline bool sqIsValid(const int sqindex) {

    return (sqindex >= 0 && sqindex <= 63);

}

// Pawns
inline unsigned int getPawnPushSq(const unsigned int sqindex, const uint64_t allPieces, const int up) {

    return (sqIsValid(sqindex + up) && (!(SQUARES[sqindex + up] & allPieces))) ? sqindex + up : NOSQ;

}

inline unsigned int getPawnDoublePushSq(const unsigned int sqindex, const uint64_t allPieces, const int up, const Color color) {

    return (sqIsValid(sqindex + (2 * up)) && (SQUARES[sqindex] & PAWN_STARTRANK[color]) && (!((SQUARES[sqindex + (2 * up)]) & allPieces)) && (!(SQUARES[sqindex + up] & allPieces))) ? sqindex + (2 * up) : NOSQ;

}

inline uint64_t generatePawnCaptures(const unsigned int sqindex, const uint64_t oppPieces, const Color color) {

    return AttackBitboards[Pawn(color)][sqindex] & oppPieces;

}

// Pawns
inline uint64_t generatePawnMoves(const Color color, const unsigned int sq, const uint64_t allPieces, const uint64_t oppPieces) {

    return (SQUARES[getPawnPushSq(sq, allPieces, DIRECTIONS[color][UP])] | SQUARES[getPawnDoublePushSq(sq, allPieces, DIRECTIONS[color][UP], color)] | generatePawnCaptures(sq, oppPieces, color));

}

inline uint64_t generatePawnQuiets(const Color color, const unsigned int sq, const uint64_t allPieces) {

    return SQUARES[getPawnPushSq(sq, allPieces, DIRECTIONS[color][UP])] | SQUARES[getPawnDoublePushSq(sq, allPieces, DIRECTIONS[color][UP], color)];

}

inline uint64_t generate_pawns_attacks(const uint64_t pawns, const Color color) {

	return (color == WHITE) ? (((pawns & ~FILE_A) << 9) | ((pawns & ~FILE_H) << 7)) : (((pawns & ~FILE_A) >> 7) | ((pawns & ~FILE_H) >> 9));

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

extern MoveList gen_quiets(const Board& board, const Color color);
extern MoveList gen_caps(const Board& board, const Color color);
extern MoveList gen_all(const Board& board, const Color color);
extern MoveList gen_legals(const Board& board, const MoveList& moves);

#endif
