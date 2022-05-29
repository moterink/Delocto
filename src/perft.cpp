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

#include "perft.hpp"
#include "movegen.hpp"
#include "timeman.hpp"
#include "uci.hpp"

// Runs a performance test to a given depth.
// This method returns the total number of nodes visited
// by traversing the search tree and counting the number of all positions
// which may occur until a given depth.
static uint64_t recursive_traverse(int depth, PerftInfo& info, Board& board) {

    if (depth == 0) return 1;

    uint64_t total = 0;

    // Generare all legal moves for the current position
    MoveList moves = generate_moves<ALL, LEGAL>(board, board.turn());

    for (unsigned i = 0; i < moves.size(); i++) {
        Move move = moves[i];

        board.do_move(move);

        // Recursive call
        uint64_t nodes = recursive_traverse(depth - 1, info, board);

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

// Starts a perft to a given depth.
// The function outputs the number of total nodes visited
std::vector<uint64_t> runPerft(const std::string fen, const Depth maxDepth) {

    std::cout << "Starting perft test to maximum depth of " << maxDepth << "..." << std::endl << std::endl;

    Board board;
    board.set_fen(fen);

    std::vector<uint64_t> results;

    TimePoint start = Clock::now();

    uint64_t nodes = 0;
    for (Depth depth = 1; depth < maxDepth+1; depth++) {
        PerftInfo info;
        info.depth = depth;
        
        TimePoint iterationStart = Clock::now();
        nodes = recursive_traverse(depth, info, board);
        Duration duration = get_time_elapsed(iterationStart);

        results.push_back(nodes);

        std::cout << "Depth " << depth << ": " << std::setw(12) << nodes << " (took " << ((float)duration / 1000.0f) <<  "s)" << std::endl;
    }

    Duration duration = get_time_elapsed(start);

    std::cout << std::endl;
    std::cout << "Perft test finished." << std::endl;
    std::cout << "Total duration: " << ((float)duration / 1000.0f) << "s" << std::endl;

    return results;

}

// A divide test is a special kind of perft test which returns the number of positions
// that occured from each root move played for a given position. A divide test runs
// to a fixed depth only.
uint64_t runDivide(const std::string fen, const Depth depth) {

    std::cout << "Starting divide test to depth " << depth << "..." << std::endl;

    PerftInfo info;
    info.depth = depth;

    Board board;
    board.set_fen(fen);

    const uint64_t nodes = recursive_traverse(depth, info, board);

    // Generate all legal moves for the root position
    MoveList moves = generate_moves<ALL, LEGAL>(board, board.turn());

    std::cout << "Total positions to depth " << depth << ": " << nodes << std::endl << std::endl;

    // Show the root moves and the resulting number of nodes for each in the console
    for (unsigned i = 0; i < moves.size(); i++) {
        std::cout << move_to_string(moves[i]) << ": " << info.divide[i] << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Divide test finished." << std::endl;

    return nodes;
}
