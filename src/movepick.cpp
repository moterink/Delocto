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

#include "movepick.hpp"

void MovePicker::valueCaptures() {

    for (unsigned int index = 0; index < caps.size; index++) {
        caps.values[index] = board.mvvlva(caps.moves[index]);
    }

}

void MovePicker::valueQuiets() {

}

void MovePicker::valueQsCaptures() {

    for (unsigned int index = 0; index < qscaps.size; index++) {
        qscaps.values[index] = board.mvvlva(qscaps.moves[index]);
    }

}

Move MovePicker::pick() {

    switch (phase) {

        case TTMove:
        case TTMoveQS:

            {
                ++phase;

                if (ttMove != MOVE_NONE && board.is_valid(ttMove)) {
                    return ttMove;
                }

            }

        case GenCaps:

            {

                ++phase;

                caps = gen_caps(board, board.turn());
                valueCaptures();

            }

        case GoodCaps:

            {

                while (caps.index < caps.size) {

                    Move best = caps.pick();

                    assert(best != MOVE_NONE);

                    if (caps.values[caps.index] < 0) {
                        break;
                    }

                    caps.index++;

                    if (best != ttMove) {
                        return best;
                    }

                }

                ++phase;

            }

        case FirstKiller:

            ++phase;

            // Note: Checking wether killer is a capture, since it could have already been picked during GoodCaps
            if (killers[0] != ttMove && killers[0] != MOVE_NONE && !board.is_capture(killers[0]) && board.is_valid(killers[0])) {
                return killers[0];
            }

        case SecondKiller:

            ++phase;

            if (killers[1] != ttMove && killers[1] != MOVE_NONE && !board.is_capture(killers[1]) && board.is_valid(killers[1])) {
                return killers[1];
            }

        case CounterMove:

            ++phase;

            if (counterMove != ttMove && counterMove != MOVE_NONE && counterMove != killers[0] && counterMove != killers[1] && !board.is_capture(counterMove) && board.is_valid(counterMove)) {
                return counterMove;
            }

        case GenQuiets:

            {
                ++phase;

                quiets = gen_quiets(board, board.turn());

                for (unsigned int index = 0; index < quiets.size; index++) {
                    quiets.values[index] = info->history[board.turn()][board.piecetype(from_sq(quiets.moves[index]))][to_sq(quiets.moves[index])];
                }

            }

        case Quiets:

            {

                while (quiets.index < quiets.size) {

                    Move best = quiets.pick();
                    assert(best != MOVE_NONE);
                    ++quiets.index;

                    if (best != ttMove && best != killers[0] && best != killers[1] && best != counterMove) {

                        return best;

                    }

                }

                ++phase;

            }

        case LoosingCaps:

            {

                while (caps.index < caps.size) {

                    Move best = caps.pick();
                    caps.index++;

                    // TODO: Check if comparison with killer even necessary? killers are not captures!
                    if (best != ttMove && best != killers[0] && best != killers[1]) {

                        return best;

                    }

                }

                return MOVE_NONE;

            }
            break;

        case GenCapsQS:

            {
                ++phase;

                qscaps = gen_caps(board, board.turn());
                valueQsCaptures();

            }

        case CapsQS:

            {

                if (qscaps.index >= qscaps.size)
                    return MOVE_NONE;

                Move best = qscaps.pick();
                qscaps.index++;
                return best;

            }

        default:

            assert(false);

    }

    assert(false);
    return false;

}
