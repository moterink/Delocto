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

#include "types.hpp"
#include "evaluate.hpp"
#include "uci.hpp"
#include "bitboards.hpp"
#include "search.hpp"

#define VERSION 0.6

int main(int argc, char* argv[]) {

    // Init Zobrist Hash Keys
    init_hashkeys();

    // Initialize king distance array
    init_king_distance();

    // Init Bitboard Lookup Tables
    init_bitboards();

    // Initialize Piece Square Tables
    init_psqt();

    // Initialize Eval
    init_eval();

    // Initialize Search
    init_search();

    std::cout << "Delocto " << VERSION << " by Moritz Terink" << std::endl << std::endl;

    Board board;
    board.set_fen("6r1/pk3p1p/3p4/1P2p3/1P1P4/PR3NP1/3K1P1P/7R w - - 0 2");
    board.see(make_move(D4, E5, NORMAL));

    uciloop(argc, argv);

    return 0;

}
