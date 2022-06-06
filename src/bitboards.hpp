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

    // Formula for getting the magic index for the given arrangement of pieces
    int index(uint64_t occupied) {
        return ((occupied & mask) * magic) >> shift;
    }

};

extern Magic BishopMagics[SQUARE_COUNT];
extern Magic RookMagics[SQUARE_COUNT];

extern Bitboard PawnAttacksSpan[COLOR_COUNT][SQUARE_COUNT];
extern Bitboard KingShelterSpan[COLOR_COUNT][SQUARE_COUNT];
extern Bitboard KingRing[COLOR_COUNT][SQUARE_COUNT];
extern Bitboard RayTable[SQUARE_COUNT][SQUARE_COUNT];
extern Bitboard LineTable[SQUARE_COUNT][SQUARE_COUNT];
extern Bitboard PawnAttacks[COLOR_COUNT][SQUARE_COUNT];
extern Bitboard PseudoAttacks[PIECETYPE_COUNT][SQUARE_COUNT];
extern Bitboard FrontFileMask[COLOR_COUNT][SQUARE_COUNT];
extern Bitboard PassedPawnMask[COLOR_COUNT][SQUARE_COUNT];
extern Bitboard BackwardPawnMask[COLOR_COUNT][SQUARE_COUNT];

extern int KingDistance[SQUARE_COUNT][SQUARE_COUNT];

extern std::string bitboard_to_string(const Bitboard bitboard);
extern void print_bitboard(const Bitboard bitboard);

template<Piecetype T>
inline Bitboard piece_attacks(const Square sq) {
    static_assert(T != PAWN);

    return PseudoAttacks[T][sq];
}

template<Piecetype T>
inline Bitboard piece_attacks(const Square sq, Bitboard occupied);

template<>
inline Bitboard piece_attacks<BISHOP>(const Square sq, const Bitboard occupied) {
    return BishopMagics[sq].attacks[BishopMagics[sq].index(occupied)];
}

template<>
inline Bitboard piece_attacks<ROOK>(const Square sq, const Bitboard occupied) {
    return RookMagics[sq].attacks[RookMagics[sq].index(occupied)];
}

template<>
inline Bitboard piece_attacks<QUEEN>(const Square sq, const Bitboard occupied) {
    return piece_attacks<BISHOP>(sq, occupied) | piece_attacks<ROOK>(sq, occupied);
}

inline Bitboard piece_attacks(const Piecetype pt, const Square sq, const Bitboard occupied) {
    assert(pt != PAWN);

    switch(pt) {
        case BISHOP:
            return piece_attacks<BISHOP>(sq, occupied);
        case ROOK:
            return piece_attacks<ROOK>(sq, occupied);
        case QUEEN:
            return piece_attacks<QUEEN>(sq, occupied);
        default:
            return PseudoAttacks[pt][sq];
    }
}

// Returns the number of moves a king would need to move between two squares on an empty board
inline uint8_t distance(const Square sq1, const Square sq2) {
    return KingDistance[sq1][sq2];
}

namespace Bitboards {
    extern void init();
}

#endif
