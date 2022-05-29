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
#include "../src/move.hpp"
#include "../src/uci.hpp"

TEST_CASE("Move validity") {
    SECTION("General rules") {
        Board board;
        board.set_fen("1n2k3/8/8/8/8/8/4P3/4K3 w - - 0 1");

        // Start square is empty
        Move move1 = make_move(SQUARE_D1, SQUARE_E2, NORMAL);
        // Tries to move opponent's piece
        Move move2 = make_move(SQUARE_B8, SQUARE_C6, NORMAL);
        // Would capture own piece
        Move move3 = make_move(SQUARE_E1, SQUARE_E2, NORMAL);

        REQUIRE(board.is_valid(move1) == false);
        REQUIRE(board.is_valid(move2) == false);
        REQUIRE(board.is_valid(move3) == false);
    }

    SECTION("Pawn specific rules") {
        Board board;
        board.set_fen("2r1k3/PP6/8/2p1pPp1/2P5/p3P1p1/1P2P1P1/4K3 w - g6 0 1");

        const Move invalidMoves[] = {
            // Pawn push blocked by friendly pawn
            make_move(SQUARE_E2, SQUARE_E3, NORMAL),
            // Pawn push blocked by opponent's pawn
            make_move(SQUARE_C4, SQUARE_C5, NORMAL),
            // Pawn cannot jump over friendly pawn
            make_move(SQUARE_E2, SQUARE_E4, NORMAL),
            // Pawn cannot jump over opponent's pawn
            make_move(SQUARE_G2, SQUARE_G4, NORMAL),
            // Pawn cannot attack empty square
            make_move(SQUARE_E2, SQUARE_F3, NORMAL),
            // Pawn cannot capture en-passant without appropriate en-passant flag
            make_move(SQUARE_F5, SQUARE_E6, ENPASSANT),
            // Cannot move to final rank without promoting
            make_move(SQUARE_A7, SQUARE_A8, NORMAL),
            // Cannot capture to final rank without promoting
            make_move(SQUARE_B7, SQUARE_C8, NORMAL),
        };

        for (const Move& move : invalidMoves) {
            DYNAMIC_SECTION("Invalid move: " << move_to_string(move)) {
                REQUIRE(board.is_valid(move) == false);
            }
        }

        const Move validMoves[] = {
            make_move(SQUARE_B2, SQUARE_B4, NORMAL),
            make_move(SQUARE_B2, SQUARE_A3, NORMAL),
            make_move(SQUARE_E3, SQUARE_E4, NORMAL),
            make_move(SQUARE_F5, SQUARE_G6, ENPASSANT),
            make_move(SQUARE_A7, SQUARE_A8, PROMOTION_QUEEN),
            make_move(SQUARE_B7, SQUARE_C8, PROMOTION_QUEEN),
        };

        for (const Move& move : validMoves) {
            DYNAMIC_SECTION("Valid move: " << move_to_string(move)) {
                REQUIRE(board.is_valid(move) == true);
            }
        }
    }

    SECTION("Castle specific rules") {
        SECTION("White") {
            Board board1, board2;
            board1.set_fen("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1");
            board2.set_fen("4k3/8/8/8/8/8/8/R3K2R w - - 0 1");

            REQUIRE(board1.is_valid(CASTLE_MOVES[WHITE][CASTLE_SHORT]) == true);
            REQUIRE(board1.is_valid(CASTLE_MOVES[WHITE][CASTLE_LONG]) == true);
            REQUIRE(board2.is_valid(CASTLE_MOVES[WHITE][CASTLE_SHORT]) == false);
            REQUIRE(board2.is_valid(CASTLE_MOVES[WHITE][CASTLE_LONG]) == false);
        }

        SECTION("Black") {
            Board board1, board2;
            board1.set_fen("r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1");
            board2.set_fen("r3k2r/8/8/8/8/8/8/4K3 b - - 0 1");

            REQUIRE(board1.is_valid(CASTLE_MOVES[BLACK][CASTLE_SHORT]) == true);
            REQUIRE(board1.is_valid(CASTLE_MOVES[BLACK][CASTLE_LONG]) == true);
            REQUIRE(board2.is_valid(CASTLE_MOVES[BLACK][CASTLE_SHORT]) == false);
            REQUIRE(board2.is_valid(CASTLE_MOVES[BLACK][CASTLE_LONG]) == false);
        }

        SECTION("Castle path blocked") {
            Board board1, board2, board3, board4;
            board1.set_fen("4k3/8/8/8/8/8/8/4K1NR w K - 0 1");
            board2.set_fen("4k3/8/8/8/8/8/8/R1B1K3 w Q - 0 1");
            board3.set_fen("4kb1r/8/8/8/8/8/8/4K3 b k - 0 1");
            board4.set_fen("r2qk3/8/8/8/8/8/8/4K3 b q - 0 1");

            REQUIRE(board1.is_valid(CASTLE_MOVES[WHITE][CASTLE_SHORT]) == false);
            REQUIRE(board2.is_valid(CASTLE_MOVES[WHITE][CASTLE_LONG]) == false);
            REQUIRE(board3.is_valid(CASTLE_MOVES[BLACK][CASTLE_SHORT]) == false);
            REQUIRE(board4.is_valid(CASTLE_MOVES[BLACK][CASTLE_LONG]) == false);
        }
    }
}

TEST_CASE("Move legality") {
    SECTION("Multiple checkers") {
        Board board;
        board.set_fen("4k3/8/8/8/1b6/5n2/8/1N2K3 w - - 0 1");
        
        Move move = make_move(SQUARE_B1, SQUARE_C3, NORMAL);

        REQUIRE(board.is_legal(move) == false);
    }

    SECTION("Bishop moves out of pin") {
        Board board;
        board.set_fen("4k3/8/8/8/1b6/8/3B4/4K3 w - - 0 1");
        
        Move move = make_move(SQUARE_D2, SQUARE_E3, NORMAL);

        REQUIRE(board.is_legal(move) == false);
    }

    SECTION("Bishop moves along pin") {
        Board board;
        board.set_fen("4k3/8/8/8/1b6/8/3B4/4K3 w - - 0 1");
        
        Move move = make_move(SQUARE_D2, SQUARE_C3, NORMAL);

        REQUIRE(board.is_legal(move) == true);
    }

    SECTION("En-passant") {
        Board board1, board2, board3;
        // Pawn is pinned by bishop
        board1.set_fen("4k3/1b6/8/3Pp3/8/5K2/8/8 w - e6 0 1");
        // Moving pawn reveals rook attack
        board2.set_fen("3rk3/8/8/3Pp3/8/8/8/3K4 w - e6 0 1");
        // Captured pawn reveals queen attack
        board3.set_fen("4k3/8/8/q2Pp2K/8/8/8/8 w - e6 0 1");

        Move move = make_move(SQUARE_D5, SQUARE_E6, ENPASSANT);

        REQUIRE(board1.is_legal(move) == false);
        REQUIRE(board2.is_legal(move) == false);
        REQUIRE(board3.is_legal(move) == false);
    }

    SECTION("Promotion reveals rook attack") {
        Board board1, board2;
        board1.set_fen("7k/r2PK3/8/8/8/8/8/8 w - - 0 1");
        board2.set_fen("2q4k/r2PK3/8/8/8/8/8/8 w - - 0 1");

        Move move = make_move(SQUARE_D7, SQUARE_D8, PROMOTION_QUEEN);

        REQUIRE(board1.is_legal(move) == false);
        REQUIRE(board2.is_legal(move) == false);
    }

    SECTION("Castle path is attacked") {
        SECTION("Short castle") {
            Board board1, board2;
            board1.set_fen("4k3/8/8/1b6/8/8/8/4K2R w K - 0 1");
            board2.set_fen("4k3/8/8/2b5/8/8/8/4K2R w K - 0 1");

            Move move = CASTLE_MOVES[WHITE][CASTLE_SHORT];

            REQUIRE(board1.is_legal(move) == false);
            REQUIRE(board2.is_legal(move) == false);
        }

        SECTION("Long castle") {
            Board board1, board2, board3;
            board1.set_fen("4k3/8/8/8/6b1/8/8/R3K3 w Q - 0 1");
            board2.set_fen("4k3/8/8/8/5b2/8/8/R3K3 w Q - 0 1");
            board3.set_fen("4k3/8/8/8/4b3/8/8/R3K3 w Q - 0 1");

            Move move = CASTLE_MOVES[WHITE][CASTLE_LONG];

            REQUIRE(board1.is_legal(move) == false);
            REQUIRE(board2.is_legal(move) == false);
            REQUIRE(board3.is_legal(move) == true);
        }
    }
}

TEST_CASE("Move gives check") {
    SECTION("Direct check") {
        Board board;
        board.set_fen("4k3/8/8/8/8/8/4B3/4K3 w - - 0 1");

        Move move = make_move(SQUARE_E2, SQUARE_B5, NORMAL);

        REQUIRE(board.gives_check(move) == true);
    }

    SECTION("Discovered check") {
        Board board;
        board.set_fen("4k3/8/8/8/8/8/4K3/4R3 w - - 0 1");

        Move move = make_move(SQUARE_E2, SQUARE_D2, NORMAL);

        REQUIRE(board.gives_check(move) == true);
    }
}