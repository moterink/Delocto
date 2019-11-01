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

#ifndef SEARCH_H
#define SEARCH_H

#include <chrono>
#include "types.hpp"
#include "board.hpp"
#include "move.hpp"
#include "movegen.hpp"

#define DELTA_MARGIN  125

enum NodeType {

    PvNode, CutNode, AllNode

};

class PvLine {

    public:

        unsigned int size = 0;
        Move line[MAX_DEPTH];

        void merge(PvLine pv);
        void append(const Move move);
        bool compare(const PvLine& pv) const;
        void clear();

};

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

typedef struct {

    bool stopped = false;
    uint64_t nodes = 0;

    Move killers[MAX_DEPTH + 1][2];
    int history[14][64];
    Move countermove[14][64];

    Move bestmove[MAX_DEPTH] = { MOVE_NONE };
    Move currentmove[MAX_DEPTH] = { MOVE_NONE };
    int eval[MAX_DEPTH];
    int value[MAX_DEPTH];

    TimePoint start;

    bool limitTime = true;

    long long idealTime = 0;
    long long maxTime = 0;

    int hashTableHits = 0;
    int depth = 0;
    int selectiveDepth = 0;

    int pvStability = 0;

} SearchInfo;

// Piece Values for Delta Pruning in Quiescence search - 100 at end for enpassant capture, where tosq == NOPIECE
static const int DeltaMaterial[15] = { 0, 0, 100, 100, 320, 320, 330, 330, 500, 500, 950, 950, 999999, 999999, 100 };
static const int FutilityMargin[6] = { 0, 100, 200, 320, 450, 590 };
static const int SeeMaterial[15]   = { 0, 0, 100, 100, 320, 320, 330, 330, 500, 500, 950, 950, 999999, 999999, 0 };
static const int RazorMargin[5]    = { 0, 300, 350, 430, 520 };

inline int value_to_tt(int value, int plies) {

    return value >= VALUE_MATE_MAX ? value + plies : value <= VALUE_MATED_MAX ? value - plies : value;

}

inline int value_from_tt(int value, int plies) {

    return value == VALUE_NONE ? VALUE_NONE : value >= VALUE_MATE_MAX ? value - plies : value <= VALUE_MATED_MAX ? value + plies : value;

}

extern void init_search();
extern const SearchStats go(Board& board, const SearchLimits& limits);

#endif
