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

#include "move.hpp"
#include "bitboards.hpp"
#include "board.hpp"

// Checks wether a given move is legal on the board.
// A move is legal if it does not leave the own king in check
bool Board::is_legal(const Move move) const {

    const int toSq   = to_sq(move);
    const int fromSq = from_sq(move);
    const int kSq    = lsb_index(pieces(stm, KING));

    // An en-passant capture is illegal, if the capturing pawn is pinned,
    // or if the captured pawn reveals a sliding attacker which attacks the king
    if (move_type(move) == ENPASSANT) {
        const unsigned capSq = toSq + DIRECTIONS[stm][DOWN];
        uint64_t occupied = (bbColors[BOTH] ^ SQUARES[fromSq] ^ SQUARES[capSq]) | SQUARES[toSq];
        return !slider_attackers(kSq, occupied, !stm);
    }

    // A castling is illegal if any of the squares on the way of the kings end square is attacked
    if (move_type(move) == CASTLING) {
        const int step = toSq > fromSq ? -1 : 1;
        for (int sq = toSq; sq != fromSq; sq += step) {
            if (sq_attacked(sq, !stm)) {
                return false;
            }
        }
    }

    // If we move the king, check if the target square is attacked.
    // We remove the king since the king might block the attack of an enemy slider
    // to the square
    if (fromSq == kSq) {
        return !sq_attacked_noking(toSq, !stm);
    }

    return !((SQUARES[fromSq] & state.kingBlockers[stm]) && !(SQUARES[toSq] & LineTable[kSq][fromSq]));

}

// Print a move to the console
// Shows move type and origin and target square
void print_move(const Move move) {

    std::cout << "From:"  << SQUARE_NAMES[from_sq(move)]
              << " To:"   << SQUARE_NAMES[to_sq(move)]
              << " Type:" << move_type(move)
              << std::endl;

}

// Print a bitboard to the console
void print_bitboard(const uint64_t bitboard) {

    int lcount, bcount;

    lcount = 0;

    // Go over each square on the bitboard
    for (bcount = 0; bcount < 64; bcount++) {
        lcount++;
        // If the bit is set, print 1 else 0
        if (bitboard & SQUARES[bcount]) {
            std::cout << "1 ";
        } else {
            std::cout << "0 ";
        }
        // If the end of the rank has been reached, print a new line
        if (lcount == 8) {
            std::cout << std::endl;
            lcount = 0;
        }
    }

    std::cout << std::endl;

}
