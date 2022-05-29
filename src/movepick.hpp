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

#ifndef MOVEPICK_H
#define MOVEPICK_H

#include "types.hpp"
#include "move.hpp"
#include "search.hpp"
#include "thread.hpp"

// Maximum value of a move in the history table
constexpr int HISTORY_VALUE_MAX = 0x4000;

struct ScoredMoveEntry {

    Move move;
    int score;

};

class ScoredMoveList : public MoveList {

    public:

        ScoredMoveList& operator=(const MoveList& list) {
            MoveList::operator=(list);
            currentIndex = 0;
            return *this;
        }

        void swap(const unsigned index1, const unsigned index2);
        ScoredMoveEntry pick();
        inline void next() { currentIndex++; }

        inline void set_score(const unsigned index, const int score) {
            scores[index] = score;
        }

    private:

        unsigned currentIndex = 0;
        std::array<int, MOVES_MAX_COUNT> scores;

};

// Stages of the move picker
enum MovePickerPhase : unsigned {

    TT_MOVE,
    GENERATE_CAPTURES,
    GOOD_CAPTURES,
    FIRST_KILLER,
    SECOND_KILLER,
    COUNTER_MOVE,
    GENERATE_QUIETS,
    QUIETS,
    BAD_CAPTURES,
    TT_MOVE_EVASIONS,
    GENERATE_EVASIONS,
    EVASIONS,
    TT_MOVE_QS,
    GENERATE_CAPTURES_QS,
    CAPTURES_QS

};

MovePickerPhase& operator++(MovePickerPhase& phase);
MovePickerPhase operator++(MovePickerPhase& phase, int);

class MovePicker {

    public:

        const Board& board;
        const SearchInfo *info;

        Move counterMove = MOVE_NONE;

        // Constructor for normal search
        MovePicker(const Board& b, SearchInfo *i, const KillerMoves& k, const HistoryTable *h, const CounterMoveTable& c, Depth plies, Move t) : board(b), info(i), killers(std::make_pair(k.first(plies), k.second(plies))), history(h) {

            ttMove = t;

            phase = board.checkers() ? TT_MOVE_EVASIONS : TT_MOVE;

            // Get the counter move if available
            if (plies > 0 && info->currentMove[plies-1] != MOVE_NONE) {
                unsigned prevSq = to_sq(info->currentMove[plies-1]);
                counterMove = c.get_move(board.owner(prevSq), board.piecetype(prevSq), prevSq);
            }

            // If we do not have a move from the transposition table, skip to evasions or good captures
            if (ttMove == MOVE_NONE) {
                ++phase;
            }

        }

        // Constructor for quiescence search
        MovePicker(const Board& b, SearchInfo * i, const HistoryTable *h, int plies, Move lastMove, Move t) : board(b), info(i), history(h) {

            // Check if there is a transposition table move available
            // In quiescence search we only allow this move if it is a capture to the square of the last move
            if (plies > 0 && lastMove != MOVE_NONE) {
                ttMove = t != MOVE_NONE && (to_sq(lastMove) == to_sq(t)) ? t : MOVE_NONE;
            }

            phase = board.checkers() ? TT_MOVE_EVASIONS : TT_MOVE_QS;

            // If we do not have a move from the transposition table, skip to evasions or captures
            if (ttMove == MOVE_NONE) {
                ++phase;
            }

        }

        void score_captures();
        void score_quiets();
        void score_evasions();

        Move pick();

    private:

        MovePickerPhase phase = TT_MOVE;
        Move ttMove = MOVE_NONE;
        const std::pair<Move, Move> killers;
        const HistoryTable *history;
        ScoredMoveList moves;
        ScoredMoveList badCaptures;

};

#endif