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

// A FEN string should be parsed correctly and be reflected in the board state
TEST_CASE("Parse FEN") {
    SECTION("Initial position") {
        Board board;
        board.set_fen(INITIAL_POSITION_FEN);

        REQUIRE(board.get_fen() == INITIAL_POSITION_FEN);
        REQUIRE(board.turn() == WHITE);
        REQUIRE(board.enpassant_square() == SQUARE_NONE);
        REQUIRE(board.plies() == 0);
        REQUIRE(board.castle_rights() == (CASTLE_WHITE_SHORT | CASTLE_WHITE_LONG | CASTLE_BLACK_SHORT | CASTLE_BLACK_LONG));
    }

    SECTION("Position with en-passant") {
        Board board;
        board.set_fen("4k3/8/8/8/3pP3/8/8/4K3 b - e3 0 1");

        REQUIRE(board.get_fen() == "4k3/8/8/8/3pP3/8/8/4K3 b - e3 0 1");
        REQUIRE(board.turn() == BLACK);
        REQUIRE(board.enpassant_square() == SQUARE_E3);
        REQUIRE(board.plies() == 0);
    }

    SECTION("Position with different castling rights") {
        Board board;
        board.set_fen("r3k2r/8/8/8/8/8/8/R3K2R w Kq - 0 1");

        REQUIRE(board.get_fen() == "r3k2r/8/8/8/8/8/8/R3K2R w Kq - 0 1");
        REQUIRE(board.turn() == WHITE);
        REQUIRE(board.enpassant_square() == SQUARE_NONE);
        REQUIRE(board.plies() == 0);
        REQUIRE(board.castle_rights() == (CASTLE_WHITE_SHORT | CASTLE_BLACK_LONG));
    }

    SECTION("Position with halfmove and fullmove") {
        Board board;
        board.set_fen("4k3/8/8/8/8/8/P7/4K3 w - - 22 32");

        REQUIRE(board.get_fen() == "4k3/8/8/8/8/8/P7/4K3 w - - 22 32");
        REQUIRE(board.turn() == WHITE);
        REQUIRE(board.enpassant_square() == SQUARE_NONE);
        REQUIRE(board.fifty_moves_count() == 22);
        REQUIRE(board.plies() == 62);
        REQUIRE(board.castle_rights() == CASTLE_NONE);
    }
}

// Moves played on the board should update the board's internal state correctly
TEST_CASE("Applying moves") {

    // When doing and then undoing a move on a board, the fens have to match
    SECTION("Move do/undo consistency") {
        static const std::array<std::string, 4> fens = {
            "4k3/8/8/8/3pP3/8/8/4K3 b - e3 0 1",
            "rn2k3/8/8/8/8/8/8/R3K3 w - - 0 1",
            "4k3/P7/8/8/8/8/8/4K3 w - - 0 1",
            "4k3/8/8/8/3p4/8/4P3/4K3 w - - 0 1"
        };

        static const Move moves[] = {
            make_move(SQUARE_D4, SQUARE_E3, ENPASSANT),
            make_move(SQUARE_A1, SQUARE_A8, NORMAL),
            make_move(SQUARE_A7, SQUARE_A8, PROMOTION_QUEEN),
            make_move(SQUARE_E2, SQUARE_E4, NORMAL),
        };

        Board board;
        for (unsigned i = 0; i < fens.size(); i++) {
            const std::string fen = fens[i];
            const Move move = moves[i];
            DYNAMIC_SECTION("FEN: " << fen << " Move: " << move_to_string(move)) {
                board.set_fen(fen);
                board.do_move(move);
                board.undo_move();
                REQUIRE(board.get_fen() == fen);
            }
        }
    }

    SECTION("Quiet move") {
        Board board;
        board.set_fen(INITIAL_POSITION_FEN);

        board.do_move(make_move(SQUARE_E2, SQUARE_E4, NORMAL));

        REQUIRE(board.is_sq_empty(SQUARE_E2));
        REQUIRE(board.pieces(WHITE, PAWN) & SQUARES[SQUARE_E4]);
        REQUIRE(board.turn() == BLACK);
        REQUIRE(board.plies() == 1);
    }

    SECTION("Capture move") {
        Board board;
        board.set_fen("rn2k3/8/8/8/8/8/8/R3K3 w - - 0 1");

        board.do_move(make_move(SQUARE_A1, SQUARE_A8, NORMAL));

        REQUIRE(board.is_sq_empty(SQUARE_A1));
        REQUIRE(board.pieces(WHITE, ROOK) == SQUARES[SQUARE_A8]);
        REQUIRE(!board.pieces(BLACK, ROOK));
    }

    SECTION("En-passant capture") {
        Board board;
        board.set_fen("4k3/8/8/8/3pP3/8/8/4K3 b - e3 0 1");

        board.do_move(make_move(SQUARE_D4, SQUARE_E3, ENPASSANT));

        REQUIRE(board.is_sq_empty(SQUARE_E4));
        REQUIRE(board.is_sq_empty(SQUARE_D4));
        REQUIRE(!board.pieces(WHITE, PAWN));
        REQUIRE(board.pieces(BLACK, PAWN) == SQUARES[SQUARE_E3]);
        REQUIRE(board.enpassant_square() == SQUARE_NONE);
    }

    SECTION("Castle move") {
        Board board;
        board.set_fen("r3k2r/8/8/8/8/8/8/R3K2R w Kq - 0 1");

        board.do_move(make_move(SQUARE_E1, SQUARE_G1, CASTLING));

        REQUIRE(board.is_sq_empty(SQUARE_E1));
        REQUIRE(board.is_sq_empty(SQUARE_H1));
        REQUIRE(board.king_square(WHITE) == SQUARE_G1);
        REQUIRE(board.pieces(WHITE, ROOK) & SQUARES[SQUARE_F1]);
        REQUIRE(!(board.castle_rights() & CASTLE_WHITE_SHORT));
    }

    SECTION("Promotion move") {
        Board board;
        board.set_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");

        board.do_move(make_move(SQUARE_A7, SQUARE_A8, PROMOTION_QUEEN));

        REQUIRE(board.is_sq_empty(SQUARE_A7));
        REQUIRE(board.pieces(WHITE, QUEEN) == SQUARES[SQUARE_A8]);
    }

    SECTION("Move which gives check") {
        Board board;
        board.set_fen("4k3/8/8/8/8/8/8/3QK3 w - - 0 1");

        board.do_move(make_move(SQUARE_D1, SQUARE_E2, NORMAL));

        REQUIRE(board.checkers() == SQUARES[SQUARE_E2]);
    }

    SECTION("Pawn move which sets en-passant square") {
        Board board;
        board.set_fen("4k3/8/8/8/3p4/8/4P3/4K3 w - - 0 1");

        board.do_move(make_move(SQUARE_E2, SQUARE_E4, NORMAL));

        REQUIRE(board.enpassant_square() == SQUARE_E3);
    }

    SECTION("King move should revoke any castle rights") {
        Board board;
        board.set_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");

        board.do_move(make_move(SQUARE_E1, SQUARE_F1, NORMAL));

        REQUIRE(board.castle_rights() == (CASTLE_BLACK_SHORT | CASTLE_BLACK_LONG));
    }

    SECTION("Castle move should revoke both castle rights") {
        Board board;
        board.set_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");

        board.do_move(make_move(SQUARE_E1, SQUARE_G1, CASTLING));

        REQUIRE(board.castle_rights() == (CASTLE_BLACK_SHORT | CASTLE_BLACK_LONG));
    }

    SECTION("Rook move should revoke the corresponding castle right") {
        Board board;
        board.set_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");

        board.do_move(make_move(SQUARE_A1, SQUARE_A2, NORMAL));

        REQUIRE(board.castle_rights() == (CASTLE_WHITE_SHORT | CASTLE_BLACK_SHORT | CASTLE_BLACK_LONG));
    }

    SECTION("Rook being captured should revoke the corresponding castle right") {
        Board board;
        board.set_fen("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");

        board.do_move(make_move(SQUARE_A8, SQUARE_A1, NORMAL));

        REQUIRE(board.castle_rights() == (CASTLE_WHITE_SHORT | CASTLE_BLACK_SHORT));
    }

    SECTION("Rook moving back should not add back revoked castle right") {
        Board board;
        board.set_fen("r3k2r/8/8/8/8/8/R7/4K2R w Kkq - 0 1");

        board.do_move(make_move(SQUARE_A2, SQUARE_A1, NORMAL));

        REQUIRE(board.castle_rights() == (CASTLE_WHITE_SHORT | CASTLE_BLACK_SHORT | CASTLE_BLACK_LONG));
    }
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