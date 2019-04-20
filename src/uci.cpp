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

static std::map<MoveType, char> promChars = { { QUEENPROM, 'q' }, { ROOKPROM, 'r' }, { BISHOPPROM, 'b' }, { KNIGHTPROM, 'n' } };

static void play_sequence(Board& board, std::string fen, std::string input, std::string::size_type ccount) {

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

static void newgame(Board& board) {

    board.set_fen(STARTFEN);
    tTable.clear();
    pawnTable.clear();
    materialTable.clear();

}

static void benchmark() {

    Board board;
    SearchLimits limits;
    limits.depth = 12;

    uint64_t nodes = 0;

    newgame(board);

    clock_t start = std::clock();

    for (int i = 0; i < 42; i++) {

        board.set_fen(BENCHMARK_FENS[i]);
        SearchStats stats = go(board, limits);
        nodes += stats.totalNodes;

    }

    clock_t end = std::clock();
    uint64_t time_elapsed = (end - start) / (CLOCKS_PER_SEC / 1000);

    std::cout << "\nTime elapsed: " << time_elapsed << std::endl;
    std::cout << "Nodes searched: " << nodes << std::endl;
    std::cout << "Nodes/second: " << 1000 * nodes / time_elapsed << std::endl;

}

static void show_information() {

    std::cout << "id name Delocto" << std::endl;
    std::cout << "id author Moritz Terink" << std::endl << std::endl;
    std::cout << "option name Hash type spin default " << DEFAULTHASHSIZE << " max " << MAXHASHSIZE << " min " << MINHASHSIZE << std::endl;
    std::cout << "option name Threads type spin default " << DEFAULTTHREADS << " max " << MAXTHREADS << " min " << MINTHREADS << std::endl;
    std::cout << "uciok" << std::endl << std::endl;

}

std::string move_to_string(const Move raw) {

    std::string move = std::string() + SQUARE_NAMES[from_sq(raw)] + SQUARE_NAMES[to_sq(raw)];

    if (is_promotion(raw)) {
        move += promChars[move_type(raw)];
    }

    return move;

}

void uciloop(int argc, char* argv[]) {

    std::string input;

    for (int i = 1; i < argc; i++) {
        input += std::string() + argv[i] + " ";
    }

    Board board;

    board.set_fen(STARTFEN);

    tTable.setSize(DEFAULTHASHSIZE);

    newgame(board);

    do {

        if (argc == 1)
            std::getline(std::cin, input);

        std::cout << std::flush;

        if (input.find("uci") == 0) {
            show_information();
        } else if (input.find("isready") == 0) {
            std::cout << "readyok" << std::endl;
        } else if (input.find("ucinewgame") == 0) {
            newgame(board);
        } else if (input.find("position startpos") == 0) {
            board.set_fen(STARTFEN);
        } else if (input.find("position startpos moves ") == 0) {
            play_sequence(board, STARTFEN, input, 24);
        } else if (input.find("position fen ") == 0) {
            const std::string::size_type end = input.find("moves ");
            if (end != std::string::npos) {
                const std::string fen = input.substr(13, end - 13);
                play_sequence(board, fen, input, end + 6);
            } else {
                board.set_fen(input.substr(13));
            }
        } else if (input.find("setoption name ") == 0) {
            if (input.find("HashSize value ") == 15) {
                tTable.setSize(std::stoi(input.substr(30)));
                tTable.clear();
            }
        } else if (input.find("go") == 0) {

            SearchLimits limits;

            std::string::size_type wtimestr, btimestr, wincstr, bincstr, movetimestr, infinitestr, depthstr;
            wtimestr = input.find("wtime");
            btimestr = input.find("btime");
            wincstr = input.find("winc");
            bincstr = input.find("binc");
            movetimestr = input.find("movetime");
            infinitestr = input.find("infinite");
            depthstr = input.find("depth");

            if (wtimestr != std::string::npos && board.turn() == WHITE) {
                limits.whiteTime = std::stoll(input.substr(wtimestr + 6));
            } else if (btimestr != std::string::npos && board.turn() == BLACK) {
                limits.blackTime = std::stoll(input.substr(btimestr + 6));
            } else if (movetimestr != std::string::npos) {
                limits.moveTime = std::stoll(input.substr(movetimestr + 9));
            }
            if (wincstr != std::string::npos && board.turn() == WHITE) {
                limits.whiteIncrement = std::stoll(input.substr(wincstr + 5));
            } else if (bincstr != std::string::npos && board.turn() == BLACK) {
                limits.blackIncrement = std::stoll(input.substr(bincstr + 5));
            }
            if (infinitestr != std::string::npos) {
                limits.infinite = true;
            } else if (depthstr != std::string::npos) {
                limits.depth = std::stoi(input.substr(depthstr + 6)) + 1;
            }

            go(board, limits);

        } else if (input.find("eval") == 0) {
            evaluateInfo(board);
        } else if (input.find("perft") == 0) {
            perftTest(10, board);
        } else if (input.find("bench") == 0) {
            benchmark();
        } else if (input.find("quit") == 0) {
            break;
        }

        input.clear();

    } while (argc == 1);

}
