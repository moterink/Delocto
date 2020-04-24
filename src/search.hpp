/*
  Delocto Chess Engine
  Copyright (c) 2018-2020 Moritz Terink

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

#include <chrono>
#include "types.hpp"
#include "board.hpp"
#include "move.hpp"
#include "movegen.hpp"

// Margins for delta/futility pruning and razoring
static const int DeltaMargin       = 100;
static const int RazorMargin       = 300;
static const int FutilityMargin[6] = { 0, 100, 200, 320, 450, 590 };

// Types of nodes visited in search
enum NodeType {

    PvNode, CutNode, AllNode

};

// A principal variation. Holds a list of all moves played in the current sequence
class PvLine {

    public:

        unsigned size = 0;
        Move line[MAX_DEPTH];

        void merge(PvLine pv);
        void append(const Move move);
        bool compare(const PvLine& pv) const;
        void clear();

};

// Search Limits given to the go() function; set time/movetime/depth limits
// specified by the user
typedef struct {

    long long moveTime = -1;
    long long time = -1;
    long long increment = 0;

    bool infinite = false;
    unsigned int depth = MAX_DEPTH;

} SearchLimits;

typedef struct {

    uint64_t totalNodes;

} SearchStats;

// Various search information variables; shows status of current search, current iteration,
// killer moves, history, bestmove and currentMove at given depth, evaluations, time management and more
typedef struct {

    bool stopped = false;
    uint64_t nodes = 0;

    Move killers[MAX_DEPTH + 1][2];
    int history[2][7][64];
    Move counterMove[2][7][64];

    Move bestMove[MAX_DEPTH] = { MOVE_NONE };
    Move currentMove[MAX_DEPTH] = { MOVE_NONE };
    int eval[MAX_DEPTH];
    int value[MAX_DEPTH];

    TimePoint start;

    bool limitTime = true;

    long long idealTime = 0;
    long long maxTime = 0;

    int hashTableHits = 0;
    int depth = 0; // Absolute depth
    int selectiveDepth = 0; // Selective depth; so quiecent search depth is included

    int pvStability = 0;

} SearchInfo;

// Piece Values for Static Exchange Evaluation
static const int SeeMaterial[7] = { 100, 320, 330, 500, 950, 999999, 0 };

// Convert a value for the transposition table, since if the value is a mate value,
// the plies until mate have to be consistent
inline int value_to_tt(int value, int plies) {

    return value >= VALUE_MATE_MAX ? value + plies : value <= VALUE_MATED_MAX ? value - plies : value;

}

// Adjust a potential mate value from the transposition table for the current number
// of plies
inline int value_from_tt(int value, int plies) {

    return value == VALUE_NONE ? VALUE_NONE : value >= VALUE_MATE_MAX ? value - plies : value <= VALUE_MATED_MAX ? value + plies : value;

}

extern void init_search();
extern const SearchStats go(Board& board, const SearchLimits& limits);

#endif
