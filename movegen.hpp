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

static const uint64_t AttackBitboards[14][64] = {
    
    { 0 },
    { 0 },
    // White pawns
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
    // Black pawns
    {
        0x40000000000000, 0xa0000000000000, 0x50000000000000, 0x28000000000000, 0x14000000000000, 0xa000000000000, 0x5000000000000, 0x2000000000000,
        0x400000000000, 0xa00000000000, 0x500000000000, 0x280000000000, 0x140000000000, 0xa0000000000, 0x50000000000, 0x20000000000,
        0x4000000000, 0xa000000000, 0x5000000000, 0x2800000000, 0x1400000000, 0xa00000000, 0x500000000, 0x200000000,
        0x40000000, 0xa0000000, 0x50000000, 0x28000000, 0x14000000, 0xa000000, 0x5000000, 0x2000000,
        0x400000, 0xa00000, 0x500000, 0x280000, 0x140000, 0xa0000, 0x50000, 0x20000,
        0x4000, 0xa000, 0x5000, 0x2800, 0x1400, 0xa00, 0x500, 0x200,
        0x40, 0xa0, 0x50, 0x28, 0x14, 0xa, 0x5, 0x2,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    },
    // White Knights    
    {
        0x20400000000000, 0x10a00000000000, 0x88500000000000, 0x44280000000000, 0x22140000000000, 0x110a0000000000, 0x8050000000000, 0x4020000000000,
        0x2000204000000000, 0x100010a000000000, 0x8800885000000000, 0x4400442800000000, 0x2200221400000000, 0x1100110a00000000, 0x800080500000000, 0x400040200000000,
        0x4020002040000000, 0xa0100010a0000000, 0x5088008850000000, 0x2844004428000000, 0x1422002214000000, 0xa1100110a000000, 0x508000805000000, 0x204000402000000,
        0x40200020400000, 0xa0100010a00000, 0x50880088500000, 0x28440044280000, 0x14220022140000, 0xa1100110a0000, 0x5080008050000, 0x2040004020000,
        0x402000204000, 0xa0100010a000, 0x508800885000, 0x284400442800, 0x142200221400, 0xa1100110a00, 0x50800080500, 0x20400040200,
        0x4020002040, 0xa0100010a0, 0x5088008850, 0x2844004428, 0x1422002214, 0xa1100110a, 0x508000805, 0x204000402,
        0x40200020, 0xa0100010, 0x50880088, 0x28440044, 0x14220022, 0xa110011, 0x5080008, 0x2040004,
        0x402000, 0xa01000, 0x508800, 0x284400, 0x142200, 0xa1100, 0x50800, 0x20400
    },
    // Black Knights
    {
        0x20400000000000, 0x10a00000000000, 0x88500000000000, 0x44280000000000, 0x22140000000000, 0x110a0000000000, 0x8050000000000, 0x4020000000000,
        0x2000204000000000, 0x100010a000000000, 0x8800885000000000, 0x4400442800000000, 0x2200221400000000, 0x1100110a00000000, 0x800080500000000, 0x400040200000000,
        0x4020002040000000, 0xa0100010a0000000, 0x5088008850000000, 0x2844004428000000, 0x1422002214000000, 0xa1100110a000000, 0x508000805000000, 0x204000402000000,
        0x40200020400000, 0xa0100010a00000, 0x50880088500000, 0x28440044280000, 0x14220022140000, 0xa1100110a0000, 0x5080008050000, 0x2040004020000,
        0x402000204000, 0xa0100010a000, 0x508800885000, 0x284400442800, 0x142200221400, 0xa1100110a00, 0x50800080500, 0x20400040200,
        0x4020002040, 0xa0100010a0, 0x5088008850, 0x2844004428, 0x1422002214, 0xa1100110a, 0x508000805, 0x204000402,
        0x40200020, 0xa0100010, 0x50880088, 0x28440044, 0x14220022, 0xa110011, 0x5080008, 0x2040004,
        0x402000, 0xa01000, 0x508800, 0x284400, 0x142200, 0xa1100, 0x50800, 0x20400
    },
    // White Bishops
    {
        0x40201008040201, 0xa0100804020100, 0x50880402010000, 0x28448201000000, 0x14224180000000, 0xa112040800000, 0x5081020408000, 0x2040810204080,
        0x4000402010080402, 0xa000a01008040201, 0x5000508804020100, 0x2800284482010000, 0x1400142241800000, 0xa000a1120408000, 0x500050810204080, 0x200020408102040,
        0x2040004020100804, 0x10a000a010080402, 0x8850005088040201, 0x4428002844820100, 0x2214001422418000, 0x110a000a11204080, 0x805000508102040, 0x402000204081020,
        0x1020400040201008, 0x810a000a0100804, 0x488500050880402, 0x8244280028448201, 0x4122140014224180, 0x20110a000a112040, 0x1008050005081020, 0x804020002040810,
        0x810204000402010, 0x40810a000a01008, 0x204885000508804, 0x182442800284482, 0x8041221400142241, 0x4020110a000a1120, 0x2010080500050810, 0x1008040200020408,
        0x408102040004020, 0x2040810a000a010, 0x102048850005088, 0x1824428002844, 0x80412214001422, 0x804020110a000a11, 0x4020100805000508, 0x2010080402000204,
        0x204081020400040, 0x102040810a000a0, 0x1020488500050, 0x18244280028, 0x804122140014, 0x804020110a000a, 0x8040201008050005, 0x4020100804020002,
        0x102040810204000, 0x102040810a000, 0x10204885000, 0x182442800, 0x8041221400, 0x804020110a00, 0x80402010080500, 0x8040201008040200
    },
    // Black Bishops
    {
        0x40201008040201, 0xa0100804020100, 0x50880402010000, 0x28448201000000, 0x14224180000000, 0xa112040800000, 0x5081020408000, 0x2040810204080,
        0x4000402010080402, 0xa000a01008040201, 0x5000508804020100, 0x2800284482010000, 0x1400142241800000, 0xa000a1120408000, 0x500050810204080, 0x200020408102040,
        0x2040004020100804, 0x10a000a010080402, 0x8850005088040201, 0x4428002844820100, 0x2214001422418000, 0x110a000a11204080, 0x805000508102040, 0x402000204081020,
        0x1020400040201008, 0x810a000a0100804, 0x488500050880402, 0x8244280028448201, 0x4122140014224180, 0x20110a000a112040, 0x1008050005081020, 0x804020002040810,
        0x810204000402010, 0x40810a000a01008, 0x204885000508804, 0x182442800284482, 0x8041221400142241, 0x4020110a000a1120, 0x2010080500050810, 0x1008040200020408,
        0x408102040004020, 0x2040810a000a010, 0x102048850005088, 0x1824428002844, 0x80412214001422, 0x804020110a000a11, 0x4020100805000508, 0x2010080402000204,
        0x204081020400040, 0x102040810a000a0, 0x1020488500050, 0x18244280028, 0x804122140014, 0x804020110a000a, 0x8040201008050005, 0x4020100804020002,
        0x102040810204000, 0x102040810a000, 0x10204885000, 0x182442800, 0x8041221400, 0x804020110a00, 0x80402010080500, 0x8040201008040200
    },
    // White Rooks
    {
        0x7f80808080808080, 0xbf40404040404040, 0xdf20202020202020, 0xef10101010101010, 0xf708080808080808, 0xfb04040404040404, 0xfd02020202020202, 0xfe01010101010101, 
        0x807f808080808080, 0x40bf404040404040, 0x20df202020202020, 0x10ef101010101010, 0x8f7080808080808, 0x4fb040404040404, 0x2fd020202020202, 0x1fe010101010101,
        0x80807f8080808080, 0x4040bf4040404040, 0x2020df2020202020, 0x1010ef1010101010, 0x808f70808080808, 0x404fb0404040404, 0x202fd0202020202, 0x101fe0101010101,
        0x8080807f80808080, 0x404040bf40404040, 0x202020df20202020, 0x101010ef10101010, 0x80808f708080808, 0x40404fb04040404, 0x20202fd02020202, 0x10101fe01010101,
        0x808080807f808080, 0x40404040bf404040, 0x20202020df202020, 0x10101010ef101010, 0x8080808f7080808, 0x4040404fb040404, 0x2020202fd020202, 0x1010101fe010101,
        0x80808080807f8080, 0x4040404040bf4040, 0x2020202020df2020, 0x1010101010ef1010, 0x808080808f70808, 0x404040404fb0404, 0x202020202fd0202, 0x101010101fe0101,
        0x8080808080807f80, 0x404040404040bf40, 0x202020202020df20, 0x101010101010ef10, 0x80808080808f708, 0x40404040404fb04, 0x20202020202fd02, 0x10101010101fe01,
        0x808080808080807f, 0x40404040404040bf, 0x20202020202020df, 0x10101010101010ef, 0x8080808080808f7, 0x4040404040404fb, 0x2020202020202fd, 0x1010101010101fe
    },
    // Black Rooks
    {
        0x7f80808080808080, 0xbf40404040404040, 0xdf20202020202020, 0xef10101010101010, 0xf708080808080808, 0xfb04040404040404, 0xfd02020202020202, 0xfe01010101010101, 
        0x807f808080808080, 0x40bf404040404040, 0x20df202020202020, 0x10ef101010101010, 0x8f7080808080808, 0x4fb040404040404, 0x2fd020202020202, 0x1fe010101010101,
        0x80807f8080808080, 0x4040bf4040404040, 0x2020df2020202020, 0x1010ef1010101010, 0x808f70808080808, 0x404fb0404040404, 0x202fd0202020202, 0x101fe0101010101,
        0x8080807f80808080, 0x404040bf40404040, 0x202020df20202020, 0x101010ef10101010, 0x80808f708080808, 0x40404fb04040404, 0x20202fd02020202, 0x10101fe01010101,
        0x808080807f808080, 0x40404040bf404040, 0x20202020df202020, 0x10101010ef101010, 0x8080808f7080808, 0x4040404fb040404, 0x2020202fd020202, 0x1010101fe010101,
        0x80808080807f8080, 0x4040404040bf4040, 0x2020202020df2020, 0x1010101010ef1010, 0x808080808f70808, 0x404040404fb0404, 0x202020202fd0202, 0x101010101fe0101,
        0x8080808080807f80, 0x404040404040bf40, 0x202020202020df20, 0x101010101010ef10, 0x80808080808f708, 0x40404040404fb04, 0x20202020202fd02, 0x10101010101fe01,
        0x808080808080807f, 0x40404040404040bf, 0x20202020202020df, 0x10101010101010ef, 0x8080808080808f7, 0x4040404040404fb, 0x2020202020202fd, 0x1010101010101fe
    },
    // White Queens
    {
        0x7fc0a09088848281, 0xbfe0504844424140, 0xdf70a82422212020, 0xef38549211101010, 0xf71c2a4988080808, 0xfb0e152444840404, 0xfd070a1222428202, 0xfe03050911214181, 
        0xc07fc0a090888482, 0xe0bfe05048444241, 0x70df70a824222120, 0x38ef385492111010, 0x1cf71c2a49880808, 0xefb0e1524448404, 0x7fd070a12224282, 0x3fe030509112141,
        0xa0c07fc0a0908884, 0x50e0bfe050484442, 0xa870df70a8242221, 0x5438ef3854921110, 0x2a1cf71c2a498808, 0x150efb0e15244484, 0xa07fd070a122242, 0x503fe0305091121,
        0x90a0c07fc0a09088, 0x4850e0bfe0504844, 0x24a870df70a82422, 0x925438ef38549211, 0x492a1cf71c2a4988, 0x24150efb0e152444, 0x120a07fd070a1222, 0x90503fe03050911,
        0x8890a0c07fc0a090, 0x444850e0bfe05048, 0x2224a870df70a824, 0x11925438ef385492, 0x88492a1cf71c2a49, 0x4424150efb0e1524, 0x22120a07fd070a12, 0x11090503fe030509, 
        0x848890a0c07fc0a0, 0x42444850e0bfe050, 0x212224a870df70a8, 0x1011925438ef3854, 0x888492a1cf71c2a, 0x844424150efb0e15, 0x4222120a07fd070a, 0x2111090503fe0305,
        0x82848890a0c07fc0, 0x4142444850e0bfe0, 0x20212224a870df70, 0x101011925438ef38, 0x80888492a1cf71c, 0x4844424150efb0e, 0x824222120a07fd07, 0x412111090503fe03,
        0x8182848890a0c07f, 0x404142444850e0bf, 0x2020212224a870df, 0x10101011925438ef, 0x8080888492a1cf7, 0x404844424150efb, 0x2824222120a07fd, 0x81412111090503fe
    },
    // Black Queens
    {
        0x7fc0a09088848281, 0xbfe0504844424140, 0xdf70a82422212020, 0xef38549211101010, 0xf71c2a4988080808, 0xfb0e152444840404, 0xfd070a1222428202, 0xfe03050911214181, 
        0xc07fc0a090888482, 0xe0bfe05048444241, 0x70df70a824222120, 0x38ef385492111010, 0x1cf71c2a49880808, 0xefb0e1524448404, 0x7fd070a12224282, 0x3fe030509112141,
        0xa0c07fc0a0908884, 0x50e0bfe050484442, 0xa870df70a8242221, 0x5438ef3854921110, 0x2a1cf71c2a498808, 0x150efb0e15244484, 0xa07fd070a122242, 0x503fe0305091121,
        0x90a0c07fc0a09088, 0x4850e0bfe0504844, 0x24a870df70a82422, 0x925438ef38549211, 0x492a1cf71c2a4988, 0x24150efb0e152444, 0x120a07fd070a1222, 0x90503fe03050911,
        0x8890a0c07fc0a090, 0x444850e0bfe05048, 0x2224a870df70a824, 0x11925438ef385492, 0x88492a1cf71c2a49, 0x4424150efb0e1524, 0x22120a07fd070a12, 0x11090503fe030509, 
        0x848890a0c07fc0a0, 0x42444850e0bfe050, 0x212224a870df70a8, 0x1011925438ef3854, 0x888492a1cf71c2a, 0x844424150efb0e15, 0x4222120a07fd070a, 0x2111090503fe0305,
        0x82848890a0c07fc0, 0x4142444850e0bfe0, 0x20212224a870df70, 0x101011925438ef38, 0x80888492a1cf71c, 0x4844424150efb0e, 0x824222120a07fd07, 0x412111090503fe03,
        0x8182848890a0c07f, 0x404142444850e0bf, 0x2020212224a870df, 0x10101011925438ef, 0x8080888492a1cf7, 0x404844424150efb, 0x2824222120a07fd, 0x81412111090503fe
    },
    // White Kings
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
    // Black Kings
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