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

#include "./catch.hpp"

#include "../src/evaluate.hpp"

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