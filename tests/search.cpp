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

#include "catch.hpp"

#include "../src/search.hpp"

TEST_CASE("KillerMoves") {
    KillerMoves killers;
    
    SECTION("should update killers") {
        Move move1 = make_move(SQUARE_E2, SQUARE_E4, NORMAL);
        killers.update(1, move1);
        REQUIRE(killers.first(1) == move1);
        REQUIRE(killers.second(1) == MOVE_NONE);

        Move move2 = make_move(SQUARE_D2, SQUARE_D4, NORMAL);
        killers.update(1, move2);
        REQUIRE(killers.first(1) == move2);
        REQUIRE(killers.second(1) == move1);
    }

    SECTION("should clear single ply") {
        Move move1 = make_move(SQUARE_E2, SQUARE_E4, NORMAL);
        killers.update(1, move1);

        Move move2 = make_move(SQUARE_E2, SQUARE_E4, NORMAL);
        killers.update(3, move2);

        killers.clear(1);

        REQUIRE(killers.first(1) == MOVE_NONE);
        REQUIRE(killers.first(3) == move2);
    }

    SECTION("should clear all") {
        Move move1 = make_move(SQUARE_E2, SQUARE_E4, NORMAL);
        killers.update(1, move1);

        Move move2 = make_move(SQUARE_E2, SQUARE_E4, NORMAL);
        killers.update(3, move2);

        killers.clear();

        REQUIRE(killers.first(1) == MOVE_NONE);
        REQUIRE(killers.first(3) == MOVE_NONE);
    }
}

TEST_CASE("HistoryTable") {
    HistoryTable table;

    SECTION("should update score") {
        table.update_score(WHITE, KNIGHT, SQUARE_C3, 32);

        REQUIRE(table.get_score(WHITE, KNIGHT, SQUARE_C3) == 1024);
    }

    SECTION("should clear all") {
        table.update_score(WHITE, KNIGHT, SQUARE_C3, 32);
        table.update_score(BLACK, BISHOP, SQUARE_A2, -32);
        table.update_score(WHITE, QUEEN, SQUARE_D5, 16);

        table.clear();

        REQUIRE(table.get_score(WHITE, KNIGHT, SQUARE_C3) == 0);
        REQUIRE(table.get_score(BLACK, BISHOP, SQUARE_A2) == 0);
        REQUIRE(table.get_score(WHITE, QUEEN, SQUARE_D5) == 0);
    }
}

TEST_CASE("CounterMoveTable") {
    CounterMoveTable table;

    SECTION("should set move") {
        const Move move = make_move(SQUARE_E2, SQUARE_E4, NORMAL);
        table.set_move(WHITE, PAWN, SQUARE_E4, move);

        REQUIRE(table.get_move(WHITE, PAWN, SQUARE_E4) == move);
    }

    SECTION("should clear all") {
        const Move move1 = make_move(SQUARE_B1, SQUARE_C3, NORMAL);
        const Move move2 = make_move(SQUARE_B3, SQUARE_A2, NORMAL);
        const Move move3 = make_move(SQUARE_D1, SQUARE_D5, NORMAL);
        table.set_move(WHITE, KNIGHT, SQUARE_C3, move1);
        table.set_move(BLACK, BISHOP, SQUARE_A2, move2);
        table.set_move(WHITE, QUEEN, SQUARE_D5, move3);

        table.clear();

        REQUIRE(table.get_move(WHITE, KNIGHT, SQUARE_C3) == MOVE_NONE);
        REQUIRE(table.get_move(BLACK, BISHOP, SQUARE_A2) == MOVE_NONE);
        REQUIRE(table.get_move(WHITE, QUEEN, SQUARE_D5) == MOVE_NONE);
    }
}