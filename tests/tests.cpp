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

#define CATCH_CONFIG_RUNNER

#include "catch.hpp"

#include "../src/types.hpp"
#include "../src/evaluate.hpp"
#include "../src/uci.hpp"
#include "../src/bitboards.hpp"
#include "../src/search.hpp"
#include "../src/uci.hpp"

static uint64_t BenchmarkResult;

int main(int argc, char* argv[]) {

    init_hashkeys();
    init_king_distance();
    init_bitboards();
    init_psqt();
    init_eval();
    init_search();

    TTable.set_size(TRANSPOSITION_TABLE_SIZE_DEFAULT);

    int result = Catch::Session().run( argc, argv );

    if (result == 0) {
        std::cout << "Benchmark: " << BenchmarkResult << std::endl;
    } else {
        std::cout << "\033[1;31mAt least one test failed! " << "\033[0m\n";
    }

    return result;

}

TEST_CASE("Move do/undo Consistency") {
    Board board;
    board.set_fen(INITIAL_POSITION_FEN);
    board.do_move(make_move(SQUARE_E2, SQUARE_E4, NORMAL));
    board.undo_move();
    REQUIRE(board.get_fen() == INITIAL_POSITION_FEN);
}

TEST_CASE("Check Check Detection") {
    Board board;
    board.set_fen("4k3/8/8/q7/8/8/8/4K3 w - - 0 1");
    REQUIRE(board.checkers() != 0);
}

TEST_CASE("Stalemate Detection") {
    Board board;
    board.set_fen("k7/8/K7/8/8/8/1R6/8 b - - 0 1");
    MoveList moves = gen_legals(board, gen_all(board, BLACK));
    REQUIRE(moves.size == 0);
}

TEST_CASE("Material Draw Detection") {
    Board board;
    board.set_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    REQUIRE(board.check_draw() == true);
}

TEST_CASE("Draw by 50-move rule Detection") {
    Board board;
    board.set_fen("k7/8/K7/8/8/8/8/2R5 b - - 100 100");
    REQUIRE(board.check_draw() == true);
}

TEST_CASE("Mate Detection") {
    Board board;
    board.set_fen("k1R5/8/K7/8/8/8/8/8 b - - 0 1");
    MoveList moves = gen_legals(board, gen_all(board, BLACK));
    REQUIRE(moves.size == 0);
}

// Check if 2 consecutive run benchmarks are returning the same number of nodes
TEST_CASE("Benchmark Consistency") {
    uint64_t bench1 = benchmark();
    uint64_t bench2 = benchmark();
    BenchmarkResult = bench1;
    REQUIRE(bench1 == bench2);
}

// Run various perfts to ensure move generator legality
TEST_CASE("Check Perft Results") {
    std::string fen1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::string fen2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    std::string fen3 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
    REQUIRE(runPerft(fen1, 5) == 4865609);
    REQUIRE(runPerft(fen2, 4) == 4085603);
    REQUIRE(runPerft(fen3, 4) == 422333);
}