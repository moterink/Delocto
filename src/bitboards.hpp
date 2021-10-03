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

#ifndef BITBOARDS_H
#define BITBOARDS_H

#include "types.hpp"

// A Magic object, containing the magic number, the magic shift,
// the attack mask and all attack bitboards for every magic index
struct Magic {

    uint64_t magic;
    uint64_t mask;
    uint64_t shift;
    uint64_t *attacks;

};

extern Magic BishopMagics[64];
extern Magic RookMagics[64];

extern Bitboard PawnAttacksSpan[2][64];
extern Bitboard KingShelterSpan[2][64];
extern Bitboard KingRing[2][64];
extern Bitboard RayTable[64][64];
extern Bitboard LineTable[64][64];
extern Bitboard PawnAttacks[2][64];
extern Bitboard KnightAttacks[64];
extern Bitboard BishopAttacks[64];
extern Bitboard RookAttacks[64];
extern Bitboard QueenAttacks[64];
extern Bitboard KingAttacks[64];
extern Bitboard FrontFileMask[2][64];
extern Bitboard PassedPawnMask[2][64];
extern Bitboard BackwardPawnMask[2][64];

extern Bitboard get_slider_attacks(const Square sq, const Bitboard occupied, const int directions[4]);
extern int get_magic_index(const Bitboard occupied, Magic *table);
extern void init_bitboards();

#endif
