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

#include "perft.hpp"
#include "movegen.hpp"

static uint64_t perft(int depth, PerftInfo& info, Board& board) {

    if (depth == 0) return 1;

    uint64_t nodes = 0;

    MoveList moves = gen_legals(board, gen_all(board, board.turn()));

    for (unsigned int mcount = 0; mcount < moves.size; mcount++) {
        board.do_move(moves.moves[mcount]);

        nodes += perft(depth - 1, info, board);
        board.undo_move();
    }

    return nodes;

}

void perftTest(const int depth, Board& board) {

    std::cout << "Starting Perft test..." << std::endl;

    for (int dcount = 1; dcount <= depth; dcount++) {
        PerftInfo info;
        clock_t start = std::clock();
        const uint64_t nodes = perft(dcount, info, board);
        std::cout << "Depth(" << dcount << "): " << nodes << std::endl;
        clock_t end = std::clock();
        long long duration = end - start;
        long long nps = (nodes * 1000) / ((duration > 0) ? duration : 1);
        std::cout << "NPS: " << nps << std::endl;
    }

    std::cout << "Perft test finished." << std::endl;

}
