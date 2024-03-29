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

#ifndef SEARCH_H
#define SEARCH_H

#include <atomic>
#include <chrono>
#include <utility>

#include "types.hpp"
#include "board.hpp"
#include "move.hpp"
#include "movegen.hpp"

// Margins for delta/futility pruning and razoring
static const Value DeltaMargin       = 100;
static const Value RazorMargin       = 300;
static const Value FutilityMargin[6] = { 0, 100, 200, 320, 450, 590 };

// Piece Values for Static Exchange Evaluation
static const Value SeeMaterial[7] = { 100, 320, 330, 500, 950, 999999, 0 };

// Types of nodes visited in search
enum NodeType {

    PvNode, CutNode, AllNode

};

// A principal variation. Holds a list of all moves played in the current sequence
class PrincipalVariation {

    private:

        unsigned size = 0;
        std::array<Move, DEPTH_MAX> line = { MOVE_NONE };

    public:

        Move best() { assert(size > 0); return line[0]; }
        Move get_move(const unsigned index) const { return line[index]; }
        unsigned length() const { return size; }
        void reset() { size = 0; }
        void update(const Move bestMove, const PrincipalVariation& pv);

};

// Search Limits given to the go() function; set time/movetime/depth limits
// specified by the user
struct SearchLimits {

    bool infinite = false;
    unsigned multiPv = 1;
    Depth depth = DEPTH_MAX;
    uint64_t nodes = 0;

    Duration moveTime = 0;
    Duration time = 0;
    Duration increment = 0;

};

// Various search information variables; shows status of current search, current iteration,
// killer moves, history, bestmove and currentMove at given depth, evaluations, time management and more
class SearchInfo {

    public:
        
        unsigned threadIndex;
        bool isMainThread;

        std::array<Move, DEPTH_MAX> bestMove = { MOVE_NONE };
        std::array<Move, DEPTH_MAX> currentMove = { MOVE_NONE };
        std::array<Value, DEPTH_MAX> eval = { 0 };
        std::array<Value, DEPTH_MAX> value = { 0 };

        TimePoint start;

        SearchLimits limits;
        
        std::array<Move, MOVES_MAX_COUNT> multiPvMoves = { MOVE_NONE };
        size_t multiPv = 0;

        Duration idealTime = 0;
        Duration maxTime = 0;

        unsigned hashTableHits = 0;
        Depth depth = 0; // Absolute depth
        Depth selectiveDepth = 0; // Selective depth; so quiescent search depth is included
        std::atomic<uint64_t> nodes{0};

        int pvStability = 0;

        void reset();

};

class KillerMoves {

    public:
        inline Move first(Depth plies) const {
            return moves[plies].first;
        }

        inline Move second(Depth plies) const {
            return moves[plies].second;
        }

        // Put the current first killer (if available) back to second killer and
        // set the new move as first killer
        inline void update(Depth plies, Move move) {
            moves[plies].second = moves[plies].first;
            moves[plies].first = move;
        }

        inline void clear(Depth plies) {
            moves[plies] = std::make_pair(MOVE_NONE, MOVE_NONE);
        }
        
        inline void clear() {
            moves.fill(std::make_pair(MOVE_NONE, MOVE_NONE));
        }

    private:
        std::array<std::pair<Move, Move>, DEPTH_MAX+1> moves;

};

template<typename T>
class ButterflyTable {

    public:
        inline T get(Color color, Piecetype type, Square square) const {
            return values[color][type][square];
        };

        inline void clear(T value = 0) {
            std::fill_n(&values[0][0][0], COLOR_COUNT * (PIECETYPE_COUNT + 1) * SQUARE_COUNT, value);
        }

    protected:
        T values[COLOR_COUNT][PIECETYPE_COUNT+1][SQUARE_COUNT] = {};

};

class HistoryTable : public ButterflyTable<int> {

    public:
        inline int get_score(Color color, Piecetype type, Square square) const {
            return ButterflyTable::get(color, type, square);
        }

        inline void update_score(Color color, Piecetype type, Square square, int delta) {
            const int currentScore = get_score(color, type, square);
            // Formula for calculating new history score
            values[color][type][square] += 32 * delta - currentScore * std::abs(delta) / 512; 
        }

};

class CounterMoveTable : public ButterflyTable<Move> {

    public:
        inline Move get_move(Color color, Piecetype type, Square square) const {
            return ButterflyTable::get(color, type, square);
        }

        inline void set_move(Color color, Piecetype type, Square square, Move move) {
            values[color][type][square] = move;
        }

        inline void clear() {
            ButterflyTable::clear(MOVE_NONE);
        }

};

namespace Search {
    extern void init();
}

#endif
