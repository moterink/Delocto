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

#include "move.hpp"
#include "bitboards.hpp"
#include "board.hpp"

bool Board::is_legal(const Move move, const Side side) const {
    
    const unsigned int tosq   = to_sq(move);
    const unsigned int fromsq = from_sq(move);
    
    if (SQUARES[fromsq] & bitboards[King(side)]) {
        if (!sq_attacked_noking(tosq, !side)) {
            return true;
        }
    } else {
        const unsigned int ksq = lsb_index(bitboards[King(side)]);        
        if (move_type(move) == ENPASSANT) {
            const unsigned int capsq = tosq + DIRECTIONS[side][DOWN];
            uint64_t occupied = (bitboards[ALLPIECES] ^ SQUARES[fromsq] ^ SQUARES[capsq]) | SQUARES[tosq];
            return side_slider_attackers(ksq, occupied, !side) == 0;
        }
        if ((SQUARES[fromsq] & state.pinned) && !(SQUARES[tosq] & LineTable[ksq][fromsq])) {
            return false;
        }
        return true;
    }
    
    return false;
    
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