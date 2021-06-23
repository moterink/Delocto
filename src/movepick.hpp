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

#include "types.hpp"
#include "move.hpp"
#include "search.hpp"
#include "thread.hpp"

// Maximum value of a move in the history table
constexpr int HISTORY_VALUE_MAX = 0x4000;

// Stages of the move picker
enum MovePickPhase : unsigned {

    TTMove, GenCaps, GoodCaps, FirstKiller, SecondKiller, CounterMove, GenQuiets, Quiets, LosingCaps, TTMoveQS, GenEvasions, Evasions, GenCapsQS, CapsQS

};

class MovePicker {

    public:

        const Thread *thread;
        const Board& board;
        const SearchInfo *info;
        unsigned phase = TTMove;

        Move counterMove = MOVE_NONE;

        // Constructor for normal search
        MovePicker(const Thread *th, const Board& b, SearchInfo *i, Depth plies, Move t) : thread(th), board(b), info(i) {

            // Set the killer moves
            killers[0] = thread->killers[plies][0];
            killers[1] = thread->killers[plies][1];

            // If we do not have a move from the transposition table, skip to generating the captures
            if (t != MOVE_NONE) {
                ttMove = t;
            } else {
                phase = GenCaps;
                /*TODO: if (board.checkers()) {
                    phase = GenEvasions;
                } else {
                    phase = GenCaps;
                }*/
            }

            // Get the counter move if available
            if (plies > 0 && info->currentMove[plies-1] != MOVE_NONE) {
                unsigned prevSq = to_sq(info->currentMove[plies-1]);
                counterMove = thread->counterMove[board.owner(prevSq)][board.piecetype(prevSq)][prevSq];
            }

        }

        // Constructor for quiescence search
        MovePicker(const Thread *th, const Board& b, SearchInfo * i, int plies, Move lastMove, Move t) : thread(th), board(b), info(i) {

            // Check if there is a hash move available and if it is a capture to the square of the last move
            if (plies > 0 && lastMove != MOVE_NONE) {
                ttMove = t != MOVE_NONE && (to_sq(lastMove) == to_sq(t)) ? t : MOVE_NONE;
            }

            // If we do not have a transposition table move, skip to evasions or captures
            phase = TTMoveQS;
            if (ttMove == MOVE_NONE) {
                if (board.checkers()) {
                    phase = GenEvasions;
                } else {
                    phase = GenCapsQS;
                }
            }

        }

        void score_captures();
        void score_quiets();
        void score_evasions();

        Move pick();

    private:

        Move killers[2];
        Move ttMove = MOVE_NONE;
        MoveList moves;
        MoveList badCaptures;

};
