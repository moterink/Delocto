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

#include "perft.hpp"
#include "movegen.hpp"
#include "uci.hpp"

// Runs a perft to a given depth.
// This method returns the total number of nodes visited
// by traversing the search tree and counting the number of all positions
// which may occur until a given depth.
static uint64_t perft(int depth, PerftInfo& info, Board& board) {

    if (depth == 0) return 1;

    unsigned long total = 0;

    // Generare all legal moves for the current position
    MoveList moves = gen_legals(board, gen_all(board, board.turn()));

    for (unsigned i = 0; i < moves.size; i++) {
        Move move = moves.moves[i];

        board.do_move(move);

        // Recursive call
        unsigned long nodes = perft(depth - 1, info, board);

        // If we are in a root node, add the number of nodes for divide
        if (depth == info.depth) {
            info.divide[i] = nodes;
        }

        // Add to the total number of nodes
        total += nodes;

        board.undo_move();
    }

    return total;

}

// Starts a perft test to a given depth.
// The function outputs the number of total nodes visited
// and divides the number for all root nodes.
void perftTest(const unsigned depth, Board& board) {

    std::cout << "Starting perft test..." << std::endl;

    PerftInfo info;
    info.depth = depth;

    const uint64_t nodes = perft(depth, info, board);

    // Generate all legal moves for the root position
    MoveList moves = gen_legals(board, gen_all(board, board.turn()));

    std::cout << "Depth(" << depth << "): " << nodes << std::endl;

    // Show the root moves and the resulting number of nodes for each in the console
    for (unsigned i = 0; i < moves.size; i++) {
        std::cout << move_to_string(moves.moves[i]) << ": " << info.divide[i] << std::endl;
    }

    std::cout << "Perft test finished." << std::endl;

}
