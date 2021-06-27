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
class PvLine {

    public:

        unsigned size = 0;
        Move line[DEPTH_MAX];

        void merge(PvLine pv);
        void append(const Move move);
        bool compare(const PvLine& pv) const;
        void clear();

};

// Search Limits given to the go() function; set time/movetime/depth limits
// specified by the user
struct SearchLimits {

    Duration moveTime = -1;
    Duration time = -1;
    Duration increment = 0;

    bool infinite = false;
    Depth depth = DEPTH_MAX;

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

        bool limitTime = true;

        Duration idealTime = 0;
        Duration maxTime = 0;

        unsigned hashTableHits = 0;
        Depth depth = 0; // Absolute depth
        Depth selectiveDepth = 0; // Selective depth; so quiecent search depth is included
        std::atomic<uint64_t> nodes{0};

        int pvStability = 0;

        void reset();
        void clear_history();
        void clear_killers();

};

extern void init_search();

#endif
