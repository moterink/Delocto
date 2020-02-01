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

#include "types.hpp"
#include "move.hpp"
#include "search.hpp"

constexpr int HISTORY_VALUE_MAX = 0x4000;

enum MovePickPhase : unsigned int {

    TTMove, GenCaps, GoodCaps, FirstKiller, SecondKiller, CounterMove, GenQuiets, Quiets, LoosingCaps, TTMoveQS, GenCapsQS, CapsQS

};

class MovePicker {

    public:

        const Board& board;
        const SearchInfo * info;
        unsigned phase = TTMove;

        Move counterMove = MOVE_NONE;

        // Constructor for normal search
        MovePicker(const Board& b, SearchInfo * i, int plies, Move t=MOVE_NONE) : board(b), info(i) {

            killers[0] = info->killers[plies][0];
            killers[1] = info->killers[plies][1];

            if (t != MOVE_NONE) {
                ttMove = t;
            } else {
                phase = GenCaps;
            }


            if (plies > 0 && info->currentmove[plies-1] != MOVE_NONE) {
                unsigned prevSq = to_sq(info->currentmove[plies-1]);
                counterMove = info->countermove[board.owner(prevSq)][board.piecetype(prevSq)][prevSq];
            }

        }

        // Constructor for quiescence search
        MovePicker(const Board& b, int plies, Move lastMove, Move t=MOVE_NONE) : board(b) {

            ttMove = t != MOVE_NONE && (to_sq(lastMove) == to_sq(t)) ? t : MOVE_NONE;

            phase = TTMoveQS;
            if (ttMove == MOVE_NONE) {
                phase = GenCapsQS;
            }

        }

        void valueCaptures();
        void valueQuiets();
        void valueQsCaptures();

        Move pick();

    private:

        Move killers[2];
        Move ttMove = MOVE_NONE;
        MoveList caps;
        MoveList gcaps;
        MoveList quiets;
        MoveList lcaps;
        MoveList moves;
        MoveList qscaps;

};
