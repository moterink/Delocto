/*
  Delocto Chess Engine
  Copyright (c) 2018-2019 Moritz Terink

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
#include "timeman.hpp"

static const uint64_t KING_START_SQ[2]       = { SQUARES[E1], SQUARES[E8] };
static const uint64_t KING_CASTLE_SQUARES[2] = { (SQUARES[G1] | SQUARES[C1]), (SQUARES[G8] | SQUARES[C8]) };

TranspositionTable tTable;
PawnTable pawnTable;
MaterialTable materialTable;

static void play_sequence(Board& board, std::string fen, std::string input, std::string::size_type start) {

    input.append(" ");
    board.set_fen(fen);
    for (unsigned c = start; c < input.size(); c++) {

        if (input[c] != ' ') {

            const unsigned fromsq = square(7 - (input[c]     - 'a'), input[c + 1] - '1');
            const unsigned tosq   = square(7 - (input[c + 2] - 'a'), input[c + 3] - '1');

            c += 4;

            MoveType type = NORMAL;

            if (input[c] != ' ') {
                type = char_to_promotion(input[c]);
            } else if ((SQUARES[fromsq] & (board.pieces(board.turn(), KING) & KING_START_SQ[board.turn()])) && (SQUARES[tosq] & KING_CASTLE_SQUARES[board.turn()])) {
                type = CASTLING;
            } else if (tosq == board.enPassant() && SQUARES[fromsq] & board.pieces(board.turn(), PAWN)) {
                type = ENPASSANT;
            }

            board.do_move(make_move(fromsq, tosq, type));
        }

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
    limits.depth = 6;

    uint64_t nodes = 0;

    newgame(board);

    TimePoint start = Clock::now();

    for (unsigned i = 0; i < 42; i++) {

        std::cout << "Position: " << (i+1) << std::endl;
        board.set_fen(BENCHMARK_FENS[i]);
        tTable.clear();
        SearchStats stats = go(board, limits);
        nodes += stats.totalNodes;

    }

    long long elapsed = get_time_elapsed(start);

    std::cout << "\nTime elapsed: " << elapsed << std::endl;
    std::cout << "Nodes searched: " << nodes << std::endl;
    std::cout << "Nodes/second: " << 1000 * nodes / elapsed << std::endl;

}

static void show_information() {

    std::cout << "id name Delocto" << std::endl;
    std::cout << "id author Moritz Terink" << std::endl << std::endl;
    std::cout << "option name Hash type spin default " << DEFAULTHASHSIZE << " max " << MAXHASHSIZE << " min " << MINHASHSIZE << std::endl;
    std::cout << "option name Threads type spin default " << DEFAULTTHREADS << " max " << MAXTHREADS << " min " << MINTHREADS << std::endl;
    std::cout << "uciok" << std::endl << std::endl;

}

void send_info(const SearchInfo* info, const PvLine& pv, const long long duration) {

    std::string pvString;
    int value = info->value[info->depth];

    for (unsigned int p = 0; p < pv.size; p++) {
        pvString += move_to_string(pv.line[p]) + " ";
    }

    std::cout << "info depth " << info->depth << " seldepth " << info->selectiveDepth;

    if (std::abs(value) >= VALUE_MATE - MAX_DEPTH) {
        std::cout << " score mate " << ((value > 0 ? VALUE_MATE - value + 1 : -VALUE_MATE - value) / 2);
    } else {
        std::cout << " score cp " << value;
    }

    std::cout << " nodes " << info->nodes << " time " << duration << " nps " << (duration != 0 ? info->nodes * 1000 / duration : info->nodes) << " pv " << pvString << std::endl;

}

void send_bestmove(const Move bestMove) {

    std::cout << "bestmove " << (bestMove != MOVE_NONE ? move_to_string(bestMove) : "none") << std::endl;

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

        if (argc <= 1)
            std::getline(std::cin, input);

        std::cout << std::flush;

        if (input.compare("uci") == 0) {
            show_information();
        } else if (input.compare("ucinewgame") == 0) {
            newgame(board);
        } else if (input.compare("isready") == 0) {
            std::cout << "readyok" << std::endl;
        } else if (input.compare("position startpos") == 0) {
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

            if (infinitestr != std::string::npos) {
                limits.infinite = true;
            } else if (depthstr != std::string::npos) {
                limits.depth = std::stoi(input.substr(depthstr + 6)) + 1;
            } else if (movetimestr != std::string::npos) {
               limits.moveTime = std::stoll(input.substr(movetimestr + 9));
            } else {
                if (wtimestr != std::string::npos && board.turn() == WHITE) {
                    limits.time = std::stoll(input.substr(wtimestr + 6));
                } else if (btimestr != std::string::npos && board.turn() == BLACK) {
                    limits.time = std::stoll(input.substr(btimestr + 6));
                }
                if (wincstr != std::string::npos && board.turn() == WHITE) {
                    limits.increment = std::stoll(input.substr(wincstr + 5));
                } else if (bincstr != std::string::npos && board.turn() == BLACK) {
                    limits.increment = std::stoll(input.substr(bincstr + 5));
                }
            }

            go(board, limits);
        } else if (input.compare("eval") == 0) {
            evaluateInfo(board);
        } else if (input.find("perft ") == 0) {
            unsigned depth = std::stoi(input.substr(6));
            perftTest(depth, board);
        } else if (input.compare("bench") == 0) {
            benchmark();
        } else if (input.compare("quit") == 0) {
            break;
        }

        input.clear();

    } while (argc <= 1);

}
