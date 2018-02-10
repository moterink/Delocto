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
uint64_t RayTable[64][64];
uint64_t LineTable[64][64];

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
        
    }
    
}