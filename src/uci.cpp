/*
  Delocto Chess Engine
  Copyright (c) 2018 Moritz Terink

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
#include "uci.hpp"
#include "search.hpp"
#include "evaluate.hpp"

static const uint64_t KING_START_SQ[2]       = { SQUARES[E1], SQUARES[E8] };
static const uint64_t KING_CASTLE_SQUARES[2] = { (SQUARES[G1] | SQUARES[C1]), (SQUARES[G8] | SQUARES[C8]) };
static std::map<char, MoveType> CharToProm   = { { 'q', QUEENPROM }, { 'r', ROOKPROM }, { 'b', BISHOPPROM }, { 'n', KNIGHTPROM } };

TranspositionTable tTable;
PawnTable pawnTable;
MaterialTable materialTable;

static void playSequence(Board& board, std::string fen, std::string input, std::string::size_type ccount);

static std::map<MoveType, char> promChars = { { QUEENPROM, 'q' }, { ROOKPROM, 'r' }, { BISHOPPROM, 'b' }, { KNIGHTPROM, 'n' } };

std::string move_to_string(const Move raw) {

    std::string move = std::string() + SQUARE_NAMES[from_sq(raw)] + SQUARE_NAMES[to_sq(raw)];

    if (is_promotion(raw)) {
        move += promChars[move_type(raw)];
    }

    return move;

}

void showInformation() {

    std::cout << "id name Delocto" << std::endl;
    std::cout << "id author Moritz Terink" << std::endl << std::endl;
    std::cout << "option name Hash type spin default " << DEFAULTHASHSIZE << " max " << MAXHASHSIZE << " min " << MINHASHSIZE << std::endl;
    std::cout << "option name Threads type spin default " << DEFAULTTHREADS << " max " << MAXTHREADS << " min " << MINTHREADS << std::endl;
    std::cout << "uciok" << std::endl << std::endl;

}

void uciloop() {

    std::string input;

    Board board;

    board.set_fen(STARTFEN);

    tTable.setSize(DEFAULTHASHSIZE);

    tTable.clear();
    pawnTable.clear();
    materialTable.clear();

    while (true) {

        std::getline(std::cin, input);

        std::cout << std::flush;

        if (input.compare("uci") == 0) {
            showInformation();
        } else if (input.compare("isready") == 0) {
            std::cout << "readyok" << std::endl;
        } else if (input.compare("ucinewgame") == 0) {
            board.set_fen(STARTFEN);
            tTable.clear();
            pawnTable.clear();
            materialTable.clear();
        } else if (input.compare("position startpos") == 0) {
            board.set_fen(STARTFEN);
        } else if (input.find("position startpos moves ") == 0) {
            playSequence(board, STARTFEN, input, 24);
        } else if (input.find("position fen ") == 0) {
            const std::string::size_type end = input.find("moves ");
            if (end != std::string::npos) {
                const std::string fen = input.substr(13, end - 13);
                playSequence(board, fen, input, end + 6);
            } else {
                board.set_fen(input.substr(13));
            }
        } else if (input.find("setoption name ") == 0) {
            if (input.find("HashSize value ") == 15) {
                tTable.setSize(std::stoi(input.substr(30)));
                tTable.clear();
            }
        } else if (input.find("go") == 0) {

            long long time = 1000;
            long long increment = 0;
            unsigned int depth = MAXDEPTH;
            unsigned int limit = TIME_LIMIT;

            std::string::size_type wtimestr, btimestr, wincstr, bincstr, movetimestr, infinitestr, depthstr;
            wtimestr = input.find("wtime");
            btimestr = input.find("btime");
            wincstr = input.find("winc");
            bincstr = input.find("binc");
            movetimestr = input.find("movetime");
            infinitestr = input.find("infinite");
            depthstr = input.find("depth");

            if (wtimestr != std::string::npos && board.turn() == WHITE) {
                time = std::stoll(input.substr(wtimestr + 6));
                limit = TIME_LIMIT;
            } else if (btimestr != std::string::npos && board.turn() == BLACK) {
                time = std::stoll(input.substr(btimestr + 6));
                limit = TIME_LIMIT;
            } else if (movetimestr != std::string::npos) {
                time = std::stoll(input.substr(movetimestr + 9));
                limit = MOVETIME_LIMIT;
            }
            if (wincstr != std::string::npos && board.turn() == WHITE) {
                increment = std::stoll(input.substr(wincstr + 5));
                limit = TIME_LIMIT;
            } else if (bincstr != std::string::npos && board.turn() == BLACK) {
                increment = std::stoll(input.substr(bincstr + 5));
                limit = TIME_LIMIT;
            }
            if (infinitestr != std::string::npos) {
                limit = INFINITE_LIMIT;
            } else if (depthstr != std::string::npos) {
                depth = std::stoi(input.substr(depthstr + 6)) + 1;
                limit = INFINITE_LIMIT;
            }

            const Move move = bestmove(board, limit, depth, time, increment, true);
            if (move != NOMOVE)
                std::cout << "bestmove " << move_to_string(move) << std::endl;

        } else if (input.compare("eval") == 0) {
            evaluateInfo(board);
        } else if (input.compare("perft") == 0) {
            perftTest(10, board);
        } else if (input.compare("quit") == 0) {
            break;
        }
    }

}

static void playSequence(Board& board, std::string fen, std::string input, std::string::size_type ccount) {

    input.append(" ");
    board.set_fen(fen);
    while (ccount < input.size()) {

        if (input[ccount] != ' ') {

            const unsigned int fromsq    = square(input[ccount]     - 'a', 8 - (input[ccount + 1] - '0'));
            const unsigned int tosq      = square(input[ccount + 2] - 'a', 8 - (input[ccount + 3] - '0'));

            ccount += 4;

            MoveType type = (input[ccount] != ' ') ? CharToProm[input[ccount]] : 0;

            if (type == 0) {

                if ((SQUARES[fromsq] & (board.pieces(KING, board.turn()) & KING_START_SQ[board.turn()])) && (SQUARES[tosq] & KING_CASTLE_SQUARES[board.turn()])) {
                    type = CASTLING;
                } else if (tosq == board.enPassant() && SQUARES[fromsq] & board.pieces(PAWN, board.turn())) {
                    type = ENPASSANT;
                } else {
                    type = NORMAL;
                }

            }

            board.do_move(make_move(fromsq, tosq, type));
        }
        ccount++;
    }

}
