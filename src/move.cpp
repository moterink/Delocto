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

    const Square toSq   = to_sq(move);
    const Square fromSq = from_sq(move);
    const Square kSq    = lsb_index(pieces(stm, KING));

    // An en-passant capture is illegal if the capturing pawn is pinned
    // or if the captured pawn reveals a sliding attacker which attacks the king
    if (move_type(move) == ENPASSANT) {
        const Square capSq = toSq + direction(stm, DOWN);
        Bitboard occupied = (bbColors[BOTH] ^ SQUARES[fromSq] ^ SQUARES[capSq]) | SQUARES[toSq];
        return !slider_attackers(kSq, occupied, !stm);
    }

    // A castle move is illegal if the kings start square, end square or any squares
    // between those two are being attacked
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
    } else if (checkers()) {
        // If there is more than one checker and we are not moving the king,
        // this move cannot be legal
        if (popcount(checkers()) >= 2) {
            return false;
        }

        // We can now assume that there is only a single checker
        const Square checkerSq         = lsb_index(checkers());
        const Bitboard blockingSquares = RayTable[checkerSq][kSq];

        // In case the checker is a slider and the move does not capture the
        // checking piece or blocks the sliding attack, then this move is illegal
        if (!((blockingSquares | checkers()) & SQUARES[toSq])) {
            return false;
        }
    }

    // If the moving piece is pinned, it may only move along the pin line
    return !(
                 (SQUARES[fromSq] & state.kingBlockers[stm])
             && !(SQUARES[toSq]   & LineTable[kSq][fromSq])
            );

}