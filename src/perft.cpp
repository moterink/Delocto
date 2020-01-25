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
#include "uci.hpp"

static uint64_t perft(int depth, PerftInfo& info, Board& board) {

    if (depth == 0) return 1;

    unsigned long total = 0;

    MoveList moves = gen_legals(board, gen_all(board, board.turn()));

    for (unsigned i = 0; i < moves.size; i++) {
        Move move = moves.moves[i];

        board.do_move(move);

        unsigned long nodes = perft(depth - 1, info, board);

        if (depth == info.depth) {
            info.divide[i] = nodes;
        }

        total += nodes;

        board.undo_move();
    }

    return total;

}

void perftTest(const unsigned depth, Board& board) {

    std::cout << "Starting perft test..." << std::endl;

    PerftInfo info;
    info.depth = depth;

    const uint64_t nodes = perft(depth, info, board);

    MoveList moves = gen_legals(board, gen_all(board, board.turn()));

    std::cout << "Depth(" << depth << "): " << nodes << std::endl;

    for (unsigned i = 0; i < moves.size; i++) {
        std::cout << move_to_string(moves.moves[i]) << ": " << info.divide[i] << std::endl;
    }

    std::cout << "Perft test finished." << std::endl;

}
