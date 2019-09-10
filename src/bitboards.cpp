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

#include "bitboards.hpp"
#include "movegen.hpp"

uint64_t PawnAttacksSpan[2][64];
uint64_t KingShelterSpan[2][64];
uint64_t KingRing[2][64];
uint64_t RayTable[64][64];
uint64_t LineTable[64][64];
uint64_t AttackBitboards[14][64];
uint64_t FrontFileMask[2][64];
uint64_t PassedPawnMask[2][64];
uint64_t BackwardPawnMask[2][64];

void initBitboards() {

    for (unsigned int sq = 0; sq < 64; sq++) {

        uint64_t pawnsFrontW = 0, pawnsFrontB = 0, kingsFrontW = SQUARES[sq], kingsFrontB = SQUARES[sq];

        for (int i = 1; i < 6; i++) {
            pawnsFrontW |= SQUARES[sq] << (i * 8);
            pawnsFrontB |= SQUARES[sq] >> (i * 8);
            kingsFrontW |= pawnsFrontW;
            kingsFrontB |= pawnsFrontB;
        }

        PawnAttacksSpan[WHITE][sq] = ((pawnsFrontW & ~FILE_A) << 1) | ((pawnsFrontW & ~FILE_H) >> 1);
        PawnAttacksSpan[BLACK][sq] = ((pawnsFrontB & ~FILE_A) << 1) | ((pawnsFrontB & ~FILE_H) >> 1);
        KingShelterSpan[WHITE][sq] = ((kingsFrontW & ~FILE_A) << 1) | ((kingsFrontW & ~FILE_H) >> 1) | kingsFrontW;
        KingShelterSpan[BLACK][sq] = ((kingsFrontB & ~FILE_A) << 1) | ((kingsFrontB & ~FILE_H) >> 1) | kingsFrontB;

        AttackBitboards[WHITE_PAWN][sq]   = ((SQUARES[sq] & ~FILE_A) << 9) | ((SQUARES[sq] & ~FILE_H) << 7);
        AttackBitboards[BLACK_PAWN][sq]   = ((SQUARES[sq] & ~FILE_A) >> 7) | ((SQUARES[sq] & ~FILE_H) >> 9);
        AttackBitboards[WHITE_KNIGHT][sq] = AttackBitboards[BLACK_KNIGHT][sq] = ((SQUARES[sq] & ~(FILE_A | RANK_8 | RANK_7)) << 17) | ((SQUARES[sq] & ~(FILE_H | RANK_8 | RANK_7)) << 15) | ((SQUARES[sq] & ~(FILE_A | FILE_B | RANK_8)) << 10) | ((SQUARES[sq] & ~(FILE_H | FILE_G | RANK_8)) << 6) | ((SQUARES[sq] & ~(FILE_A | FILE_B | RANK_1)) >> 6) | ((SQUARES[sq] & ~(FILE_H | FILE_G | RANK_1)) >> 10) | ((SQUARES[sq] & ~(FILE_A | RANK_1 | RANK_2)) >> 15) | ((SQUARES[sq] & ~(FILE_H | RANK_1 | RANK_2)) >> 17);
        AttackBitboards[WHITE_BISHOP][sq] = AttackBitboards[BLACK_BISHOP][sq] = generateBishopMoves(sq, 0, 0);
        AttackBitboards[WHITE_ROOK][sq]   = AttackBitboards[BLACK_ROOK][sq]   = generateRookMoves(sq, 0, 0);
        AttackBitboards[WHITE_QUEEN][sq]  = AttackBitboards[BLACK_QUEEN][sq]  = AttackBitboards[WHITE_ROOK][sq] | AttackBitboards[WHITE_BISHOP][sq];
        AttackBitboards[WHITE_KING][sq]   = AttackBitboards[BLACK_KING][sq]   = ((SQUARES[sq] & ~(FILE_A | RANK_8)) << 9) | ((SQUARES[sq] & ~RANK_8) << 8) | ((SQUARES[sq] & ~(FILE_H | RANK_8)) << 7) | ((SQUARES[sq] & ~FILE_A) << 1) | ((SQUARES[sq] & ~FILE_H) >> 1) | ((SQUARES[sq] & ~(FILE_A | RANK_1)) >> 7) | ((SQUARES[sq] & ~RANK_1) >> 8) | ((SQUARES[sq] & ~(FILE_H | RANK_1)) >> 9);

        KingRing[WHITE][sq] = AttackBitboards[WHITE_KING][sq];
        KingRing[BLACK][sq] = AttackBitboards[BLACK_KING][sq];
        if (relative_rank(WHITE, sq) == 0) {
            KingRing[WHITE][sq] |= shift_up(KingRing[WHITE][sq], WHITE);
        }
        if (relative_rank(BLACK, sq) == 0) {
            KingRing[BLACK][sq] |= shift_up(KingRing[BLACK][sq], BLACK);
        }
        if (file(sq) == 0) {
            KingRing[WHITE][sq] |= shift_right(KingRing[WHITE][sq], WHITE);
            KingRing[BLACK][sq] |= shift_left(KingRing[BLACK][sq], BLACK);
        }
        if (file(sq) == 7) {
            KingRing[WHITE][sq] |= shift_left(KingRing[WHITE][sq], WHITE);
            KingRing[BLACK][sq] |= shift_right(KingRing[BLACK][sq], BLACK);
        }

        for (int i = 1; i < 8; i++) {
            uint64_t nsq = (SQUARES[sq] << (8 * i));
            FrontFileMask[WHITE][sq] |= nsq;
            if (nsq & RANK_8)
                break;
        }

        for (int i = 1; i < 8; i++) {
            uint64_t nsq = (SQUARES[sq] >> (8 * i));
            FrontFileMask[BLACK][sq] |= nsq;
            if (nsq & RANK_1)
                break;
        }

    }

    for (unsigned int sq1 = 0; sq1 < 64; sq1++) {

        const uint64_t bishopPseudoBB = AttackBitboards[BISHOP][sq1];
        const uint64_t rookPseudoBB   = AttackBitboards[ROOK][sq1];

        for (unsigned int sq2 = 0; sq2 < 64; sq2++) {

            if (bishopPseudoBB & SQUARES[sq2]) {
                RayTable[sq1][sq2]  = (generateBishopMoves(sq1, SQUARES[sq2], SQUARES[sq2]) & generateBishopMoves(sq2, SQUARES[sq1], SQUARES[sq1])) | SQUARES[sq2];
                LineTable[sq1][sq2] = (generateBishopMoves(sq1, 0, 0) & generateBishopMoves(sq2, 0, 0)) | SQUARES[sq1] | SQUARES[sq2];
            } else if (rookPseudoBB & SQUARES[sq2]) {
                RayTable[sq1][sq2]  = (generateRookMoves(sq1, SQUARES[sq2], SQUARES[sq2]) & generateRookMoves(sq2, SQUARES[sq1], SQUARES[sq1])) | SQUARES[sq2];
                LineTable[sq1][sq2] = (generateRookMoves(sq1, 0, 0) & generateRookMoves(sq2, 0, 0)) | SQUARES[sq1] | SQUARES[sq2];
            }

        }

        int f = file(sq1);
        int r = rank(sq1);
        PassedPawnMask[WHITE][sq1] = FrontFileMask[WHITE][sq1] | (f != 0 ? FrontFileMask[WHITE][sq1-1] : 0) | (f != 7 ? FrontFileMask[WHITE][sq1+1] : 0);
        PassedPawnMask[BLACK][sq1] = FrontFileMask[BLACK][sq1] | (f != 0 ? FrontFileMask[BLACK][sq1-1] : 0) | (f != 7 ? FrontFileMask[BLACK][sq1+1] : 0);

        BackwardPawnMask[WHITE][sq1] = (r != 0 ? (f != 0 ? FrontFileMask[BLACK][sq1-9] : 0) | (f != 7 ? FrontFileMask[BLACK][sq1-7] : 0) : 0);
        BackwardPawnMask[BLACK][sq1] = (r != 7 ? (f != 0 ? FrontFileMask[WHITE][sq1+7] : 0) | (f != 7 ? FrontFileMask[WHITE][sq1+9] : 0) : 0);

    }

}
