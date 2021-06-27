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
#include "uci.hpp"
#include "search.hpp"
#include "evaluate.hpp"
#include "timeman.hpp"
#include "thread.hpp"

static const uint64_t KING_START_SQ[2]       = { SQUARES[SQUARE_E1], SQUARES[SQUARE_E8] };
static const uint64_t KING_CASTLE_SQUARES[2] = { (SQUARES[SQUARE_G1] | SQUARES[SQUARE_C1]), (SQUARES[SQUARE_G8] | SQUARES[SQUARE_C8]) };

unsigned ThreadsCount = 1;
unsigned MoveOverhead = 100;

ThreadPool Threads(ThreadsCount);

TranspositionTable TTable;

// This function takes a FEN and a principal variation string as an input and
// plays the given sequence of moves on a board.
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
            } else if (tosq == board.enpassant_square() && SQUARES[fromsq] & board.pieces(board.turn(), PAWN)) {
                type = ENPASSANT;
            }

            board.do_move(make_move(fromsq, tosq, type));
        }

    }

}

// Sets up a new game for the given board; also clears all hash tables
static void newgame(Board& board) {

    board.set_fen(INITIAL_POSITION_FEN);
    TTable.clear();
    Threads.reset();

}

// Find the best move for the given position.
// The search can be constrained by depth or time or nodes.
static void go(const Board& board, const SearchLimits& limits) {

    // If we are already searching, stop the current search
    if (!Threads.has_stopped()) {
        Threads.stop_searching();
    }
    
    Threads.wait_until_finished();

    Threads.initialize_search(board, limits);
    Threads.start_searching();
    
}

// Runs a benchmark for 42 testing positions to retrieve a number of total visited
// nodes. This number should stay consistent and can be used as a verification
// of search/evaluation integrity
uint64_t benchmark() {

    Board board;
    SearchLimits limits;
    limits.depth = 6;

    uint64_t nodes = 0;

    // Set thread count to 1 for consistency
    Threads.resize(1);
    Threads.reset();

    TimePoint start = Clock::now();

    for (unsigned i = 0; i < 42; i++) {

        std::cout << "Position: " << (i + 1) << std::endl;
        board.set_fen(BENCHMARK_FENS[i]);
        TTable.clear(); // Clear Transposition Table between searches
        go(board, limits);

        // Wait for search thread to finish
        Threads.wait_until_finished();
        nodes += Threads.get_nodes();

    }
    
    Duration elapsed = get_time_elapsed(start);

    std::cout << std::endl;
    std::cout << "========== BENCHMARK FINISHED ==========" << std::endl;
    std::cout << "Time elapsed (ms):          " << std::setw(12) << elapsed << std::endl;
    std::cout << "Nodes searched (total):     " << std::setw(12) << nodes << std::endl;
    std::cout << "Nodes searched (per second):" << std::setw(12) << 1000 * nodes / elapsed << std::endl << std::endl;

    return nodes;

}

// Send the engine identification and options to the console; end it with a "uciok"
static void show_information() {

    std::cout << "id name Delocto " << VERSION << std::endl;
    std::cout << "id author Moritz Terink" << std::endl << std::endl;
    std::cout << "option name Hash type spin default " << TRANSPOSITION_TABLE_SIZE_DEFAULT << " max " << TRANSPOSITION_TABLE_SIZE_MAX << " min " << TRANSPOSITION_TABLE_SIZE_MIN << std::endl;
    std::cout << "option name Threads type spin default " << THREADS_DEFAULT << " max " << THREADS_MAX << " min " << THREADS_MIN << std::endl;
    std::cout << "option name MoveOverhead type spin default " << MOVE_OVERHEAD_DEFAULT << " max " << MOVE_OVERHEAD_MAX << " min " << MOVE_OVERHEAD_MIN << std::endl;
    std::cout << "uciok" << std::endl << std::endl;

}

// This function receives various information about the current search iteration and prints
// information like current depth, selective depth, duration, score... to the console. It
// also shows a principal variation (the suggested line of play)
void send_info(const SearchInfo& info, const PvLine& pv, const Duration duration, const uint64_t nodes) {

    std::string pvString;
    Value value = info.value[info.depth];

    // Connect the moves of the principal variation to a string
    for (unsigned pvIndex = 0; pvIndex < pv.size; pvIndex++) {
        pvString += move_to_string(pv.line[pvIndex]) + " ";
    }

    // Show depth and selective depth 
    // (using unary operator + because uint8_t is treated like a character by std::cout)
    std::cout << "info depth " << info.depth
              << " seldepth "  << info.selectiveDepth;

    // Show the score (in centipawns) or if a mate was found, the plies until the mate
    if (std::abs(value) >= VALUE_MATE_MAX) {
        std::cout << " score mate " << ((value > 0 ? VALUE_MATE - value + 1 : -VALUE_MATE - value) / 2);
    } else {
        std::cout << " score cp " << value;
    }

    // Show number of visited nodes, duration, nodes per second, and the principal variation
    std::cout << " nodes "    << nodes
              << " time "     << duration 
              << " nps "      << (duration != 0 ? nodes * 1000 / duration : nodes)
              << " hashfull " << TTable.hashfull()
              << " pv "       << pvString << std::endl;

}

// Show the best move for the current position in the console; if the position is mate or stalemate, the engine will output "none"
void send_bestmove(const Move bestMove) {

    std::cout << "bestmove " << (bestMove != MOVE_NONE ? move_to_string(bestMove) : "none") << std::endl;

}

// Parse a string and act according to the UCI protocol
bool parse_uci_input(std::string input, Board& board) {

    // Show information about the engine
    if (input.compare("uci") == 0) {
        show_information();
    // Reset the board tot the initial position
    } else if (input.compare("ucinewgame") == 0) {
        newgame(board);
    // A sort of ping command sent by the user interface to check if the engine is still responsive
    } else if (input.compare("isready") == 0) {
        std::cout << "readyok" << std::endl;
    // Set the board to the initial position
    } else if (input.compare("position startpos") == 0) {
        board.set_fen(INITIAL_POSITION_FEN);
    // Play all the moves from the initial position
    } else if (input.find("position startpos moves ") == 0) {
        play_sequence(board, INITIAL_POSITION_FEN, input, 24);
    // Set the position to the given FEN; play all the moves on the board
    } else if (input.find("position fen ") == 0) {
        const std::string::size_type end = input.find("moves ");
        if (end != std::string::npos) {
            const std::string fen = input.substr(13, end - 13);
            play_sequence(board, fen, input, end + 6);
        } else {
            board.set_fen(input.substr(13));
        }
    // Set an option
    } else if (input.find("setoption name ") == 0) {
        if (input.find("Hash value ") == 15) {
            TTable.set_size(std::max(1, std::stoi(input.substr(26))));
            TTable.clear();
        } else if (input.find("Threads value ") == 15) {
            ThreadsCount = std::stoi(input.substr(29));
            Threads.resize(ThreadsCount);
        } else if (input.find("MoveOverhead value ") == 15) {
            MoveOverhead = std::stoi(input.substr(34));
        }
    // Start the search
    } else if (input.find("go") == 0) {
        SearchLimits limits;
        std::string::size_type wtimestr, btimestr, wincstr, bincstr, movetimestr, infinitestr, depthstr;

        // Information about white/black time, increments, fixed depth, etc.
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
            limits.depth = std::min(std::stoi(input.substr(depthstr + 6)) + 1, DEPTH_MAX);
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
    // Interrupt the current search
    } else if (input.compare("stop") == 0) {
        Threads.stop_searching();
    // Output an evaluation of the current position. Useful for debugging
    } else if (input.compare("eval") == 0) {
        evaluate_info(board);
    // Run a perft test
    } else if (input.find("perft ") == 0) {
        unsigned depth = std::stoi(input.substr(6));
        runPerft(board.get_fen(), depth);
    // Run a benchmark
    } else if (input.compare("bench") == 0) {
        benchmark();
    // Quit the program
    } else if (input.compare("quit") == 0) {
        // If we are currently searching, stop it before quitting
        if (!Threads.has_stopped()) {
            Threads.stop_searching();
            Threads.wait_until_finished();
        }
        return true;
    }

    // Parsing done, input was not quit
    return false;

}

// The uci input loop. It scans the input for uci commands and executes them
void uci_loop(int argc, char* argv[]) {

    std::string input;

    // Set the transposition table to the default size in Megabytes; can be changed through options later
    TTable.set_size(TRANSPOSITION_TABLE_SIZE_DEFAULT);

    // Initialize a chess board with the initial position
    Board board;
    newgame(board);

    // If we received command line arguments, only execute them
    // and then quit immediately
    if (argc > 1) {
        parse_uci_input(argv[1], board);
    } else {
        bool shouldQuit = false;
        while (!shouldQuit) {

            std::getline(std::cin, input);
            shouldQuit = parse_uci_input(input, board);

        };
    }

}