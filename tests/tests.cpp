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

    TTable.set_size(HashOption.get_default());

    int result = Catch::Session().run( argc, argv );

    if (result == 0) {
        std::cout << "Benchmark: " << BenchmarkResult << std::endl;
    } else {
        std::cout << "\033[1;31mAt least one test failed! " << "\033[0m\n";
    }

    return result;

}

// When doing and then undoing a move on a board, the fens have to match
TEST_CASE("Move do/undo consistency") {
    Board board;
    board.set_fen(INITIAL_POSITION_FEN);
    board.do_move(make_move(SQUARE_E2, SQUARE_E4, NORMAL));
    board.undo_move();
    REQUIRE(board.get_fen() == INITIAL_POSITION_FEN);
}

// A position where the king is in check should return a bitboard with checkers
TEST_CASE("Check detection") {
    static const std::string fens[] = {
        "6k1/6pp/8/8/8/8/5pPP/6K1 w - - 0 1",
        "6k1/6pp/8/8/8/8/4n1PP/6K1 w - - 0 1",
        "6k1/6pp/1b6/8/8/8/6PP/6K1 w - - 0 1",
        "6k1/6pp/8/8/8/8/6PP/1r4K1 w - - 0 1",
        "6k1/6pp/8/2q5/8/8/6PP/6K1 w - - 0 1",
        "6k1/5Ppp/8/8/8/8/6PP/6K1 b - - 0 1",
        "6k1/4N1pp/8/8/8/8/6PP/6K1 b - - 0 1",
        "6k1/6pp/8/8/8/1B6/6PP/6K1 b - - 0 1",
        "1R4k1/6pp/8/8/8/8/6PP/6K1 b - - 0 1",
        "6k1/6pp/8/8/2Q5/8/6PP/6K1 b - - 0 1"
    };

    Board board;
    for (const std::string fen : fens) {
        DYNAMIC_SECTION("FEN: " << fen) {
            board.set_fen(fen);
            REQUIRE(board.checkers());
        }
    }
    
}

// A draw by insufficient material has to be detected
TEST_CASE("Material draw detection") {
    static const std::string fens[] = {
        "4k3/8/8/8/8/8/8/4K1N1 w - - 0 1",
        "4k3/8/8/8/8/8/8/4KB2 w - - 0 1",
        "4k1n1/8/8/8/8/8/8/4K3 b - - 0 1",
        "4kb2/8/8/8/8/8/8/4K3 b - - 0 1"
    };

    Board board;
    for (const std::string fen : fens) {
        DYNAMIC_SECTION("FEN: " << fen) {
            board.set_fen(fen);
            REQUIRE(board.check_draw() == true);
        }
    }
}

// A draw by 50-move rule has to be detected
TEST_CASE("Draw by 50-move rule detection") {
    Board board;
    board.set_fen("k7/8/K7/8/8/8/8/2R5 b - - 100 100");
    REQUIRE(board.check_draw() == true);
}

// A mate position should return zero legal moves
TEST_CASE("Mate detection") {
    static const std::string fens[] = {
        "7k/6pp/8/8/8/8/6PP/1r5K w - - 0 1",
        "1R5k/6pp/8/8/8/8/6PP/7K b - - 0 1",
        "7k/7p/8/3b4/8/8/7P/1r4QK w - - 0 1",
        "1R4qk/7p/8/4B3/8/8/7P/7K b - - 0 1",
        "7k/7p/7r/8/8/6n1/6P1/6QK w - - 0 1",
        "6qk/6p1/6N1/8/8/7R/7P/7K b - - 0 1"
    };

    Board board;
    for (const std::string fen : fens) {
        DYNAMIC_SECTION("FEN: " << fen) {
            board.set_fen(fen);
            MoveList moves = gen_legals(board, gen_all(board, BLACK));
            REQUIRE(moves.size == 0);
        }
    }
    
}

// A stalemate position should return zero legal moves
TEST_CASE("Stalemate detection") {
    Board board;
    board.set_fen("k7/8/K7/8/8/8/1R6/8 b - - 0 1");
    MoveList moves = gen_legals(board, gen_all(board, BLACK));
    REQUIRE(moves.size == 0);
}

// Two evaluation calls for the same position need to return the same value
TEST_CASE("Evaluation consistency") {
    static const std::string fens[] = {
        "q3kb1Q/3p1pr1/p3p2B/1p1bP3/2rN4/P1P2p2/1P4PP/R3R1K1 b - - 1 24",
        "1k1r3r/ppqn1p2/2pbpn1p/P2pN3/1P1P1P2/2PBP2p/6PP/R1BQ2K1 w - - 0 16",
        "r3kb1r/1p1n1pp1/p1p1pnp1/2Pp4/1P1P1P2/2N1P3/1P1B2PP/R3KB1R b KQkq - 0 14",
        "r1bqk2r/p5pp/2pbp3/5pB1/3P4/5N2/PP3PPP/R2Q1RK1 b kq - 3 12"
    };

    Board board;
    for (const std::string fen : fens) {
        DYNAMIC_SECTION("FEN: " << fen) {
            board.set_fen(fen);
            const Value value1 = evaluate(board, 0);
            const Value value2 = evaluate(board, 0);
            REQUIRE(value1 == value2);
        }
    }
}

// Two consecutive run benchmarks should return the same number of nodes
TEST_CASE("Benchmark consistency") {
    uint64_t bench1 = benchmark();
    uint64_t bench2 = benchmark();
    BenchmarkResult = bench1;
    REQUIRE(bench1 == bench2);
}

// Run multiple perfts to ensure move generator legality
TEST_CASE("Check perft results") {
    std::string fen1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::string fen2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    std::string fen3 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
    REQUIRE(runPerft(fen1, 5) == 4865609);
    REQUIRE(runPerft(fen2, 4) == 4085603);
    REQUIRE(runPerft(fen3, 4) == 422333);
}