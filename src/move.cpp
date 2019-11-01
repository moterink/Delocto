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

#include "move.hpp"
#include "bitboards.hpp"
#include "board.hpp"

bool Board::is_legal(const Move move) const {

    const int tosq   = to_sq(move);
    const int fromsq = from_sq(move);
    const int ksq    = lsb_index(bitboards[King(stm)]);

    if (move_type(move) == ENPASSANT) {
        const unsigned capsq = tosq + DIRECTIONS[stm][DOWN];
        uint64_t occupied = (bitboards[ALLPIECES] ^ SQUARES[fromsq] ^ SQUARES[capsq]) | SQUARES[tosq];
        return !color_slider_attackers(ksq, occupied, !stm);
    }

    if (move_type(move) == CASTLING) {
        const int step = tosq > fromsq ? -1 : 1;
        for (int sq = tosq; sq != fromsq; sq += step) {
            if (sq_attacked(sq, !stm))
                return false;
        }
    }

    if (fromsq == ksq)
        return !sq_attacked_noking(tosq, !stm);

    return !((SQUARES[fromsq] & state.kingBlockers[stm]) && !(SQUARES[tosq] & LineTable[ksq][fromsq]));

}

// Print move to stdout
void print_move(const Move move) {

    std::cout << "From:" << SQUARE_NAMES[from_sq(move)] << " To:" << SQUARE_NAMES[to_sq(move)] << " Type:" << move_type(move) << std::endl;

}

// Print bitboard to stdout
void print_bitboard(const uint64_t bitboard) {

    int lcount, bcount;

    lcount = 0;

    for (bcount = 0; bcount < 64; bcount++) {
        lcount++;
        if (bitboard & SQUARES[bcount]) {
            std::cout << "1 ";
        } else {
            std::cout << "0 ";
        }
        if (lcount == 8) {
            std::cout << std::endl;
            lcount = 0;
        }
    }

    std::cout << std::endl;

}
