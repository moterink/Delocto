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

#include "movepick.hpp"

// Score all the moves with the Most Valuable Victim - Least Valuable Attacker Heuristic
void MovePicker::score_captures(MoveList& captures) {

    for (unsigned index = 0; index < captures.size; index++) {
        captures.values[index] = board.mvvlva(captures.moves[index]);
    }

}

// Assign each move a score from the history table
void MovePicker::score_quiets() {

    for (unsigned index = 0; index < quiets.size; index++) {
        quiets.values[index] = info->history[board.turn()][board.piecetype(from_sq(quiets.moves[index]))][to_sq(quiets.moves[index])];
    }

}

// Assign each evasion a score
void MovePicker::score_evasions() {

    for (unsigned index = 0; index < evasions.size; index++) {
        if (board.is_capture(evasions.moves[index])) {
            evasions.values[index] = board.mvvlva(evasions.moves[index]);
        } else {
            evasions.values[index] = info->history[board.turn()][board.piecetype(from_sq(evasions.moves[index]))][to_sq(evasions.moves[index])];
        }

    }

}

// Pick the move with the highest chance of being the best move or causing a beta-cutoff
// The moves are picked in such a way so that we have to search as few moves as possible
// reducing the number of nodes that need to be searched and so drastically decreasing the
// size of the search tree.
Move MovePicker::pick() {

    switch (phase) {

        // Transposition Table Move
        case TTMove:
        case TTMoveQS:

            {
                ++phase;

                // First, try the move from the transposition table. Check if it is valid in the current position
                if (ttMove != MOVE_NONE && board.is_valid(ttMove)) {
                    return ttMove;
                }

            }

        case GenCaps:

            {

                ++phase;

                // Generate all captures for the current position and score them
                caps = gen_caps(board, board.turn());
                score_captures(caps);

            }

        // Good Captures
        // These moves gain material in the next move
        case GoodCaps:

            {

                while (caps.index < caps.size) {

                    Move best = caps.pick();

                    assert(best != MOVE_NONE);

                    // If the capture loses material, move to the next stage
                    if (caps.values[caps.index] < 0) {
                        break;
                    }

                    caps.index++;

                    // The capture could be equal to the transposition table move
                    if (best != ttMove) {
                        return best;
                    }

                }

                ++phase;

            }

        // Killer Moves
        // These are quiet moves which have caused a beta-cutoff in the search in previous iterations, so it is wise to try them again
        case FirstKiller:

            ++phase;

            // NOTE: Checking wether killer is a capture, since it could have already been picked during GoodCaps
            if (killers[0] != ttMove && killers[0] != MOVE_NONE && !board.is_capture(killers[0]) && board.is_valid(killers[0])) {
                return killers[0];
            }

        case SecondKiller:

            ++phase;

            if (killers[1] != ttMove && killers[1] != MOVE_NONE && !board.is_capture(killers[1]) && board.is_valid(killers[1])) {
                return killers[1];
            }

        // Counter Move
        // A counter move is move which works good to counter a piece at a certain square,
        // so next we try that
        case CounterMove:

            ++phase;

            // Check that we do not have tried the counter move in a previous state already
            if (counterMove != ttMove && counterMove != MOVE_NONE && counterMove != killers[0] && counterMove != killers[1] && !board.is_capture(counterMove) && board.is_valid(counterMove)) {
                return counterMove;
            }

        // Quiet moves
        case GenQuiets:

            {
                ++phase;

                // Generate all quiet moves and assign them a value based on their history score
                quiets = gen_quiets(board, board.turn());
                score_quiets();

            }

        case Quiets:

            {

                while (quiets.index < quiets.size) {

                    // Pick the quiet move with the highest score
                    Move best = quiets.pick();
                    assert(best != MOVE_NONE);
                    ++quiets.index;

                    // Make sure we have not tried this quiet move before
                    if (best != ttMove && best != killers[0] && best != killers[1] && best != counterMove) {
                        return best;
                    }

                }

                ++phase;

            }

        case LosingCaps:

            {

                // Next, we try captures which lose material in the next move.
                // These moves are usually quite bad so we try them last
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

        // Evasions
        // These are moves which escape the check
        case GenEvasions:

            {

                ++phase;

                // Generate all evasions (both quiet and captures, also in quiescence search)
                // and score them with either mvv-lva or history values
                gen_evasions(board, MOVES_ALL, evasions, board.turn());
                score_evasions();

            }

        case Evasions:

            {

                while (evasions.index < evasions.size) {

                    // Pick the best evasion
                    Move best = evasions.pick();
                    assert(best != MOVE_NONE);
                    ++evasions.index;

                    if (best != ttMove) {
                        return best;
                    }

                }

                // Do not continue with captures in quiescence search; all captures evading check have already been searched in Evasions
                return MOVE_NONE;

            }

        // Captures in quiescence search
        case GenCapsQS:

            {
                ++phase;

                // Same as state GenCaps, only in quiescence search
                qscaps = gen_caps(board, board.turn());
                score_captures(qscaps);

            }

        case CapsQS:

            {

                if (qscaps.index >= qscaps.size)
                    return MOVE_NONE;

                // Pick the best move
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