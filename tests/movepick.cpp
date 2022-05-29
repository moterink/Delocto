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

#include "../src/movepick.hpp"

TEST_CASE("MovePicker") {
    SECTION("Position 1") {
        static const Move moves[] = {
            make_move(SQUARE_C5, SQUARE_F5, NORMAL),
            make_move(SQUARE_C5, SQUARE_C6, NORMAL),
            make_move(SQUARE_C5, SQUARE_A7, NORMAL),
            make_move(SQUARE_F1, SQUARE_D2, NORMAL),
            make_move(SQUARE_F1, SQUARE_E3, NORMAL),
            make_move(SQUARE_F4, SQUARE_C1, NORMAL),
            make_move(SQUARE_F4, SQUARE_D2, NORMAL),
            make_move(SQUARE_F4, SQUARE_E3, NORMAL),
            make_move(SQUARE_F4, SQUARE_G5, NORMAL),
            make_move(SQUARE_F4, SQUARE_E5, NORMAL),
            make_move(SQUARE_F4, SQUARE_H6, NORMAL),
            make_move(SQUARE_F4, SQUARE_D6, NORMAL),
            make_move(SQUARE_F4, SQUARE_C7, NORMAL),
            make_move(SQUARE_F4, SQUARE_B8, NORMAL),
            make_move(SQUARE_A1, SQUARE_E1, NORMAL),
            make_move(SQUARE_A1, SQUARE_D1, NORMAL),
            make_move(SQUARE_A1, SQUARE_C1, NORMAL),
            make_move(SQUARE_A1, SQUARE_B1, NORMAL),
            make_move(SQUARE_C5, SQUARE_C1, NORMAL),
            make_move(SQUARE_C5, SQUARE_C2, NORMAL),
            make_move(SQUARE_C5, SQUARE_E3, NORMAL),
            make_move(SQUARE_C5, SQUARE_C3, NORMAL),
            make_move(SQUARE_C5, SQUARE_A3, NORMAL),
            make_move(SQUARE_C5, SQUARE_D4, NORMAL),
            make_move(SQUARE_C5, SQUARE_C4, NORMAL),
            make_move(SQUARE_C5, SQUARE_B4, NORMAL),
            make_move(SQUARE_C5, SQUARE_H5, NORMAL),
            make_move(SQUARE_C5, SQUARE_G5, NORMAL),
            make_move(SQUARE_C5, SQUARE_E5, NORMAL),
            make_move(SQUARE_C5, SQUARE_D5, NORMAL),
            make_move(SQUARE_C5, SQUARE_B5, NORMAL),
            make_move(SQUARE_C5, SQUARE_A5, NORMAL),
            make_move(SQUARE_C5, SQUARE_D6, NORMAL),
            make_move(SQUARE_C5, SQUARE_B6, NORMAL),
            make_move(SQUARE_C5, SQUARE_E7, NORMAL),
            make_move(SQUARE_C5, SQUARE_F8, NORMAL),
            make_move(SQUARE_H2, SQUARE_H3, NORMAL),
            make_move(SQUARE_F2, SQUARE_F3, NORMAL),
            make_move(SQUARE_B2, SQUARE_B3, NORMAL),
            make_move(SQUARE_A2, SQUARE_A3, NORMAL),
            make_move(SQUARE_G3, SQUARE_G4, NORMAL),
            make_move(SQUARE_D3, SQUARE_D4, NORMAL),
            make_move(SQUARE_H2, SQUARE_H4, NORMAL),
            make_move(SQUARE_B2, SQUARE_B4, NORMAL),
            make_move(SQUARE_A2, SQUARE_A4, NORMAL),
            make_move(SQUARE_G1, SQUARE_H1, NORMAL),
            make_move(SQUARE_G1, SQUARE_G2, NORMAL),
            make_move(SQUARE_G3, SQUARE_G4, NORMAL)
        };

        Board board;
        board.set_fen("2r1q2k/pp4pp/2n2b2/2Q5/5B2/3P2P1/PP3P1P/R4NK1 w - - 1 21");

        Move ttMove = make_move(SQUARE_C5, SQUARE_F5, NORMAL);

        SearchInfo info;

        KillerMoves killers;
        HistoryTable historyTable;
        CounterMoveTable cmTable;

        MovePicker picker(board, &info, killers, &historyTable, cmTable, 4, ttMove);

        Move bestMove;
        unsigned index = 0;

        while ((bestMove = picker.pick()) != MOVE_NONE) {
            REQUIRE(bestMove == moves[index]);
            index++;
        }
    }
}

TEST_CASE("ScoredMoveList") {
    ScoredMoveList moves;
    const Move move1 = make_move(SQUARE_E2, SQUARE_E4, NORMAL);
    const Move move2 = make_move(SQUARE_D2, SQUARE_D4, NORMAL);
    const Move move3 = make_move(SQUARE_C2, SQUARE_C4, NORMAL);
    moves.append(move1);
    moves.append(move2);
    moves.append(move3);
    moves.set_score(0, 10);
    moves.set_score(1, -2);
    moves.set_score(2, 28);

    ScoredMoveEntry best;

    SECTION("should pick the highest score") {
        best = moves.pick();
        moves.next();
        REQUIRE(best.move == move3);
        REQUIRE(best.score == 28);

        best = moves.pick();
        moves.next();
        REQUIRE(best.move == move1);
        REQUIRE(best.score == 10);

        best = moves.pick();
        moves.next();
        REQUIRE(best.move == move2);
        REQUIRE(best.score == -2);

        best = moves.pick();
        REQUIRE(best.move == MOVE_NONE);
    }
}