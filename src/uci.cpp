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
#include <sstream>
#include <stdlib.h>

#include "uci.hpp"
#include "search.hpp"
#include "evaluate.hpp"
#include "timeman.hpp"
#include "thread.hpp"

SpinOption   ThreadsOption      = SpinOption("Threads", 1, 1, 4);
SpinOption   HashOption         = SpinOption("Hash", 64, 1, 4096);
ButtonOption ClearHashOption    = ButtonOption("Clear Hash", [] { TTable.clear(); });
SpinOption   MoveOverheadOption = SpinOption("MoveOverhead", 100, 0, 10000);
SpinOption   MultiPVOption      = SpinOption("MultiPV", 1, 1, 100);

const Option* Options[5] = {
    &ThreadsOption,
    &HashOption,
    &ClearHashOption,
    &MoveOverheadOption,
    &MultiPVOption,
};

ThreadPool Threads(ThreadsOption.get_default());
TranspositionTable TTable;

// This function receives various information about the current search iteration and prints
// information like current depth, selective depth, duration, score... to the console. It
// also shows a principal variation (the suggested line of play)
void send_pv(const SearchInfo& info, const Value value, const PrincipalVariation& pv, const uint64_t nodes, const Value alpha, const Value beta) {

    std::stringstream ss;
    Duration duration = get_time_elapsed(info.start);

    // Show depth and selective depth
    ss << "info depth " << info.depth
       << " seldepth "  << info.selectiveDepth;

    // If we are in multiPV mode, also send the pv index
    if (info.limits.multiPv > 1) {
        ss << " multipv " << info.multiPv + 1;
    }

    // Show the score (in centipawns) or if a mate was found, the plies until the mate
    if (std::abs(value) >= VALUE_MATE_MAX) {
        ss << " score mate " << (value > 0 ? VALUE_MATE - value + 1 : -VALUE_MATE - value) / 2;
    } else {
        ss << " score cp " << value;
    }

    if (value >= beta) {
        ss << " lowerbound";
    } else if (value <= alpha) {
        ss << " upperbound";
    }

    // Show number of visited nodes, duration, nodes per second, and the principal variation
    ss << " nodes "    << nodes
       << " time "     << duration
       << " nps "      << (duration != 0 ? nodes * 1000 / duration : nodes)
       << " hashfull " << TTable.hashfull();

    // Connect the moves of the principal variation to a string
    if (pv.length()) {
        ss << " pv";
        for (unsigned pvIndex = 0; pvIndex < pv.length(); pvIndex++) {
            ss << ' ' << move_to_string(pv.get_move(pvIndex));
        }
    }

    ss << std::endl;
    
    std::cout << ss.str();

}

void send_string(const std::string string) {
    
    std::cout << "info string " << string << std::endl;

}

void send_currmove(const Move currentMove, const unsigned index) {

    std::cout << "info currmove " << move_to_string(currentMove) << " currmovenumber " << index << std::endl;

}

// Show the best move for the current position in the console; if the position is mate or stalemate, the engine will output "none"
void send_bestmove(const Move bestMove) {

    std::cout << "bestmove " << (bestMove != MOVE_NONE ? move_to_string(bestMove) : "none") << std::endl;

}

// Send the engine identification and options to the console; end it with a "uciok"
static void show_information() {

    std::cout << "id name Delocto " << VERSION << std::endl;
    std::cout << "id author Moritz Terink" << std::endl << std::endl;

    for (const Option* option : Options) {
        std::cout << option->uci_string() << std::endl;
    }
    
    std::cout << "uciok" << std::endl << std::endl;

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
    limits.depth = 8;

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

    // Reset thread pool to original value
    Threads.resize(ThreadsOption.get_value());

    return nodes;

}

static void handle_setoption(std::stringstream& ss) {

    std::string word, name, valueRaw;

    ss >> word;

    while (ss >> word) {
        if (word == "value") {
            break;
        }
        name += word + ' ';
    }
    name.pop_back();

    ss >> valueRaw;
    bool isValid = false;

    if (name == HashOption.name) {
        int value = std::stoi(valueRaw);
        isValid = HashOption.set_value(value);
        if (isValid) {
            TTable.set_size(value);
            TTable.clear();
        }
    } else if (name == ThreadsOption.name) {
        int value = std::stoi(valueRaw);
        isValid = ThreadsOption.set_value(value);
        if (isValid) {
            Threads.resize(value);
        }
    } else if (name == MoveOverheadOption.name) {
        isValid = MoveOverheadOption.set_value(std::stoi(valueRaw));
    } else if (name == MultiPVOption.name) {
        isValid = MultiPVOption.set_value(std::stoi(valueRaw));
    } else if (name == ClearHashOption.name) {
        isValid = true;
        ClearHashOption.push();
    } else {
        send_string("Error: No option named \"" + name + "\"");
        return;
    }

    if (!isValid) {
        send_string("Error: Invalid value for option " + name);
    }

}

static void handle_position(std::stringstream& ss, Board& board) {

    std::stringstream position;
    std::string part;

    while (ss >> part && part != "moves") {
        if (part == "startpos") {
            position << INITIAL_POSITION_FEN;
        } else if (part == "fen") {
            continue;
        } else {
            position << part << ' ';
        }
    }

    board.set_fen(position.str());

    std::string move;

    while (ss >> move) {

        const Square fromSq = square(7 - (move[0] - 'a'), move[1] - '1');
        const Square toSq   = square(7 - (move[2] - 'a'), move[3] - '1');

        MoveType type = NORMAL;

        if (move.length() == 5) {
            type = char_to_promotion(move[4]);
        } else if (    (SQUARES[fromSq] & board.pieces(board.turn(), KING) & SQUARES[KING_INITIAL_SQUARE[board.turn()]])
                    && std::abs(toSq - fromSq) == 2) {
            type = CASTLING;
        } else if (toSq == board.enpassant_square() && SQUARES[fromSq] & board.pieces(board.turn(), PAWN)) {
            type = ENPASSANT;
        }

        board.do_move(make_move(fromSq, toSq, type));

    }

}

static void handle_go(std::stringstream& ss, Board& board) {

    SearchLimits limits;
    std::string part;

    // Information about white/black time, increments, fixed depth, etc.
    while (ss >> part) {
        if (part == "infinite") {
            limits.infinite = true;
            break;
        } else if (part == "depth") {
            ss >> part;
            limits.depth = std::min(std::stoi(part), DEPTH_MAX);
            break;
        } else if (part == "nodes") {
            ss >> part;
            limits.nodes = std::max(std::stoll(part), 1ll);
            break;
        } else if (part == "movetime") {
            ss >> part;
            limits.moveTime = std::stoll(part);
            break;
        } else if ((part == "wtime" && board.turn() == WHITE) || (part == "btime" && board.turn() == BLACK)) {
            ss >> part;
            limits.time = std::stoll(part);
        } else if ((part == "winc" && board.turn() == WHITE) || (part == "binc" && board.turn() == BLACK)) {
            ss >> part;
            limits.increment = std::stoll(part);
        }
    }

    limits.multiPv = MultiPVOption.get_value();

    go(board, limits);

}

// Parse a string and act according to the UCI protocol
bool parse_uci_input(std::string input, Board& board) {

    std::stringstream ss(input);
    std::string word;

    while (ss >> word) {
        // Show information about the engine
        if (word == "uci") {
            show_information();
            break;
        }

        // Reset the board tot the initial position
        if (word == "ucinewgame") {
            newgame(board);
            break;
        }
        
        // A sort of ping command sent by the user interface to check if the engine is still responsive
        if (word == "isready") {
            std::cout << "readyok" << std::endl;
            break;
        }

        if (word == "setoption") {
            handle_setoption(ss);
            break;
        }

        if (word == "position") {
            handle_position(ss, board);
            break;
        }

        if (word == "go") {
            handle_go(ss, board);
            break;
        }

        // Interrupt the current search
        if (word == "stop") {
            Threads.stop_searching();
            break;
        }

        // Output an evaluation of the current position. Useful for debugging
        if (word == "eval") {
            evaluate_info(board);
            break;
        }

        // Run a perft test
        if (word == "perft") {
            unsigned depth = std::stoi(input.substr(6));
            runPerft(board.get_fen(), depth);
            break;
        }

        // Run a benchmark
        if (word == "bench") {
            benchmark();
            break;
        }

        // Quit the program
        if (word == "quit") {
            // If we are currently searching, stop it before quitting
            if (!Threads.has_stopped()) {
                Threads.stop_searching();
                Threads.wait_until_finished();
            }
            return true;
        }
        
    }

    // Parsing done, input was not quit
    return false;

}

// The uci input loop. It scans the input for uci commands and executes them
void uci_loop(int argc, char* argv[]) {

    std::string input;

    // Set the transposition table to the default size in Megabytes; can be changed through options later
    TTable.set_size(HashOption.get_default());

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

        }
    }

}