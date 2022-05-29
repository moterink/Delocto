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

#include "../src/board.hpp"

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