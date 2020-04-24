/*
  Delocto Chess Engine
  Copyright (c) 2018-2020 Moritz Terink

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
#include "bitboards.hpp"

class MoveList {

    public:

        unsigned size = 0;
        unsigned index = 0;

        Move moves[250];
        int values[250];

        inline void append(Move move) {
            moves[size++] = move;
        }

        void merge(MoveList list);
        unsigned find(const Move move);
        void swap(const unsigned int index1, const unsigned int index2);
        Move pick();
        void print();

};

// Pawns
// Returns the index of the single push square for a pawn on the given square for the given color
inline unsigned get_pawn_push_sq(const unsigned sq, const uint64_t allPieces, const int up) {

    return (sq_valid(sq + up) && (!(SQUARES[sq + up] & allPieces))) ? sq + up : SQUARE_NONE;

}

// Returns the index of the double push square for a pawn on the given square for the given color
inline unsigned get_pawn_double_push_sq(const unsigned sq, const uint64_t allPieces, const int up, const Color color) {

    return (sq_valid(sq + (2 * up)) && (SQUARES[sq] & PAWN_STARTRANK[color]) && (!((SQUARES[sq + (2 * up)]) & allPieces)) && (!(SQUARES[sq + up] & allPieces))) ? sq + (2 * up) : SQUARE_NONE;

}

// Returns a bitboard of all squares a pawn on the given square can capture pieces for the given color
inline uint64_t generate_pawn_captures(const unsigned sq, const uint64_t oppPieces, const Color color) {

    return PawnAttacks[color][sq] & oppPieces;

}

// Returns a bitboard of all possible pawn destination squares on the given square for the given color
inline uint64_t generate_pawn_moves(const Color color, const unsigned sq, const uint64_t allPieces, const uint64_t oppPieces) {

    return (SQUARES[get_pawn_push_sq(sq, allPieces, DIRECTIONS[color][UP])] | SQUARES[get_pawn_double_push_sq(sq, allPieces, DIRECTIONS[color][UP], color)] | generate_pawn_captures(sq, oppPieces, color));

}

// Returns a bitboard of all attack squares for a pawn on the given square for the given color
inline uint64_t generate_pawns_attacks(const uint64_t pawns, const Color color) {

	return (color == WHITE) ? (((pawns & ~BB_FILE_A) << 9) | ((pawns & ~BB_FILE_H) << 7)) : (((pawns & ~BB_FILE_A) >> 7) | ((pawns & ~BB_FILE_H) >> 9));

}

// Returns a bitboard of all possible destination squares for a knight on the given square
inline uint64_t gen_knight_moves(const unsigned sq, const uint64_t ownPieces) {

    return KnightAttacks[sq] & ~ownPieces;

}

// Returns a bitboard of all possible destination squares for a king on the given square
inline uint64_t gen_king_moves(const unsigned sq, const uint64_t ownPieces) {

    return KingAttacks[sq] & ~ownPieces;

}

// Sliding Pieces
// Returns a bitboard of all possible destination squares for a bishop on the given square
inline uint64_t gen_bishop_moves(const unsigned sq, const uint64_t both, const uint64_t friendly) {

    return BishopMagics[sq].attacks[get_magic_index(both, &BishopMagics[sq])] & ~friendly;

}

// Returns a bitboard of all possible destination squares for a rook on the given square
inline uint64_t gen_rook_moves(const unsigned sq, const uint64_t both, const uint64_t friendly) {

    return RookMagics[sq].attacks[get_magic_index(both, &RookMagics[sq])] & ~friendly;

}

// Returns a bitboard of all possible destination squares for a queen on the given square
inline uint64_t get_queen_moves(const unsigned int sq, const uint64_t both, const uint64_t friendly) {

    return gen_bishop_moves(sq, both, friendly) | gen_rook_moves(sq, both, friendly);

}

extern MoveList gen_quiets(const Board& board, const Color color);
extern MoveList gen_caps(const Board& board, const Color color);
extern void gen_evasions(const Board&board, const MoveGenType mtype, MoveList& moveList, const Color color);
extern MoveList gen_all(const Board& board, const Color color);
extern MoveList gen_legals(const Board& board, const MoveList& moves);

#endif
