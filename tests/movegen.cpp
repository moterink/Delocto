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

#include "../src/board.hpp"
#include "../src/movegen.hpp"

TEST_CASE("MoveList") {
    SECTION("should append correctly") {
        MoveList moves;
        const Move move = make_move(SQUARE_E2, SQUARE_E4, NORMAL);
        moves.append(move);
        REQUIRE(moves.size() == 1);
        REQUIRE(moves[0] == move);
    }

    SECTION("should concat another list") {
        MoveList moves1, moves2;
        const Move move1 = make_move(SQUARE_E2, SQUARE_E4, NORMAL);
        const Move move2 = make_move(SQUARE_D2, SQUARE_D4, NORMAL);

        moves1.append(move1);
        moves2.append(move2);

        moves1.concat(moves2);
        
        REQUIRE(moves1.size() == 2);
        REQUIRE(moves1[0] == move1);
        REQUIRE(moves1[1] == move2);
    }

    SECTION("should swap moves") {
        MoveList moves;
        const Move move1 = make_move(SQUARE_E2, SQUARE_E4, NORMAL);
        const Move move2 = make_move(SQUARE_D2, SQUARE_D4, NORMAL);

        moves.append(move1);
        moves.append(move2);

        moves.swap(0, 1);

        REQUIRE(moves[0] == move2);
        REQUIRE(moves[1] == move1);
    }
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
            MoveList moves = generate_moves<ALL, LEGAL>(board, board.turn());
            REQUIRE(moves.size() == 0);
        }
    }
    
}

// A stalemate position should return zero legal moves
TEST_CASE("Stalemate detection") {
    Board board;
    board.set_fen("k7/8/K7/8/8/8/1R6/8 b - - 0 1");
    MoveList moves = generate_moves<ALL, LEGAL>(board, BLACK);
    REQUIRE(moves.size() == 0);
}

TEST_CASE("Generate pseudolegal moves") {
    SECTION("All moves") {
        static const std::array<std::string, 1> fens = {
            "4k3/8/8/8/1b6/5n2/8/1N2K3 w - - 0 1"
        };
        static const std::vector<Move> moves[] = {
            {
                make_move(SQUARE_B1, SQUARE_D2, NORMAL),
                make_move(SQUARE_B1, SQUARE_C3, NORMAL),
                make_move(SQUARE_B1, SQUARE_A3, NORMAL),
                make_move(SQUARE_E1, SQUARE_F1, NORMAL),
                make_move(SQUARE_E1, SQUARE_D1, NORMAL),
                make_move(SQUARE_E1, SQUARE_F2, NORMAL),
                make_move(SQUARE_E1, SQUARE_E2, NORMAL),
                make_move(SQUARE_E1, SQUARE_D2, NORMAL),
            }
        };

        Board board;
        for (unsigned f = 0; f < fens.size(); f++) {
            DYNAMIC_SECTION("FEN: " << fens[f]) {
                board.set_fen(fens[f]);
                MoveList receivedMoves = generate_moves<ALL, PSEUDO_LEGAL>(board, board.turn());
                REQUIRE(receivedMoves.size() == moves[f].size());
                for (unsigned m = 0; m < moves[f].size(); m++) {
                    //std::cout << move_to_string(receivedMoves[m]) << ":" << move_to_string(moves[f][m]) << std::endl;
                    REQUIRE(receivedMoves[m] == moves[f][m]);
                }
            }
        }
    }

    SECTION("Evasions") {
        static const std::array<std::string, 1> fens = {
            "4k3/8/8/8/1b6/5n2/8/1N2K3 w - - 0 1"
        };
        static const std::vector<Move> moves[] = {
            {
                make_move(SQUARE_E1, SQUARE_F1, NORMAL),
                make_move(SQUARE_E1, SQUARE_D1, NORMAL),
                make_move(SQUARE_E1, SQUARE_F2, NORMAL),
                make_move(SQUARE_E1, SQUARE_E2, NORMAL),
                make_move(SQUARE_E1, SQUARE_D2, NORMAL),
            }
        };

        Board board;
        for (unsigned f = 0; f < fens.size(); f++) {
            DYNAMIC_SECTION("FEN: " << fens[f]) {
                board.set_fen(fens[f]);
                MoveList receivedMoves = generate_moves<EVASION, PSEUDO_LEGAL>(board, board.turn());
                REQUIRE(receivedMoves.size() == moves[f].size());
                for (unsigned m = 0; m < moves[f].size(); m++) {
                    REQUIRE(receivedMoves[m] == moves[f][m]);
                }
            }
        }
    }
    
}

TEST_CASE("Generate legal moves") {
    static const std::array<std::string, 2> fens = {
        "r1bqkbnr/pppppppp/8/8/8/2N1Pn2/PPPP1PPP/R1BQKB1R w KQkq - 0 1",
        "4k3/8/8/8/1b6/5n2/8/1N2K3 w - - 0 1"
    };
    static const std::vector<Move> moves[] = {
        { make_move(SQUARE_E1, SQUARE_E2, NORMAL), make_move(SQUARE_G2, SQUARE_F3, NORMAL), make_move(SQUARE_D1, SQUARE_F3, NORMAL) },
        { make_move(SQUARE_E1, SQUARE_F1, NORMAL), make_move(SQUARE_E1, SQUARE_D1, NORMAL), make_move(SQUARE_E1, SQUARE_F2, NORMAL), make_move(SQUARE_E1, SQUARE_E2, NORMAL) }
    };

    Board board;
    for (unsigned f = 0; f < fens.size(); f++) {
        DYNAMIC_SECTION("FEN: " << fens[f]) {
            board.set_fen(fens[f]);
            MoveList receivedMoves = generate_moves<ALL, LEGAL>(board, board.turn());
            REQUIRE(receivedMoves.size() == moves[f].size());
            for (unsigned m = 0; m < moves[f].size(); m++) {
                REQUIRE(receivedMoves[m] == moves[f][m]);
            }
        }
    }
}

