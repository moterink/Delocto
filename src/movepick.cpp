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

#include "movepick.hpp"

// Score all the moves with the Most Valuable Victim - Least Valuable Attacker Heuristic
void MovePicker::score_captures() {

    for (unsigned index = 0; index < moves.size(); index++) {
        moves.set_score(index, board.mvvlva(moves[index]));
    }

}

// Assign each move a score from the history table
void MovePicker::score_quiets() {

    for (unsigned index = 0; index < moves.size(); index++) {
        moves.set_score(index, history->get_score(board.turn(), board.piecetype(from_sq(moves[index])), to_sq(moves[index])));
    }

}

// Assign each evasion a score
void MovePicker::score_evasions() {

    for (unsigned index = 0; index < moves.size(); index++) {
        if (board.is_capture(moves[index])) {
            moves.set_score(index, board.mvvlva(moves[index]));
        } else {
            moves.set_score(index, history->get_score(board.turn(), board.piecetype(from_sq(moves[index])), to_sq(moves[index])));
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
        case TT_MOVE:
        case TT_MOVE_QS:
        case TT_MOVE_EVASIONS:

            {
                
                ++phase;

                // First, try the move from the transposition table. Check if it is valid in the current position
                if (ttMove != MOVE_NONE && board.is_valid(ttMove)) {
                    return ttMove;
                } else {
                    // Skip to next stage if there is no transposition move available
                    return pick();
                }

            }

        case GENERATE_CAPTURES:

            {

                ++phase;

                // Generate all captures for the current position and score them
                moves = generate_moves<CAPTURE, PSEUDO_LEGAL>(board, board.turn());
                score_captures();

            }

        // Good captures
        // These moves gain material in the next move
        case GOOD_CAPTURES:

            {

                ScoredMoveEntry best;
                while ((best = moves.pick()).move != MOVE_NONE) {
                    // If the capture loses material, move to the next stage
                    if (best.score < 0) {
                        badCaptures = moves;
                        break;
                    }

                    moves.next();

                    // The capture could be equal to the transposition table move
                    if (best.move != ttMove) {
                        return best.move;
                    }
                }

                ++phase;

            }

        // Killer Moves
        // These are quiet moves which have caused a beta-cutoff in the search in previous iterations, so it is wise to try them again
        case FIRST_KILLER:

            ++phase;

            // NOTE: Checking wether killer is a capture, since it could have already been picked during good captures
            if (   killers.first != ttMove
                && killers.first != MOVE_NONE
                && !board.is_capture(killers.first)
                && board.is_valid(killers.first))
            {
                return killers.first;
            }

        case SECOND_KILLER:

            ++phase;

            if (   killers.second != ttMove
                && killers.second != MOVE_NONE
                && !board.is_capture(killers.second)
                && board.is_valid(killers.second))
            {
                return killers.second;
            }

        // Counter Move
        // A counter move is move which works good to counter a piece at a certain square,
        // so next we try that
        case COUNTER_MOVE:

            ++phase;

            // Verify that we haven't tried the counter move in a previous phase already
            if (   counterMove != ttMove
                && counterMove != MOVE_NONE
                && counterMove != killers.first && counterMove != killers.second
                && !board.is_capture(counterMove)
                && board.is_valid(counterMove))
            {
                return counterMove;
            }

        // Quiet moves
        case GENERATE_QUIETS:

            {
                ++phase;

                // Generate all quiet moves and assign them a value based on their history score
                moves = generate_moves<QUIET, PSEUDO_LEGAL>(board, board.turn());
                score_quiets();

            }

        case QUIETS:

            {

                ScoredMoveEntry best;
                // Pick the quiet move with the highest score
                while ((best = moves.pick()).move != MOVE_NONE) {
                    moves.next();
                    // Make sure we have not tried this quiet move before
                    if (   best.move != ttMove
                        && best.move != killers.first && best.move != killers.second
                        && best.move != counterMove) {
                        return best.move;
                    }
                };

                ++phase;

            }

        case BAD_CAPTURES:

            {

                // Next, we try captures which lose material in the next move.
                // These moves are usually quite bad so we try them last
                ScoredMoveEntry best;
                while ((best = moves.pick()).move != MOVE_NONE) {
                    moves.next();
                    // TODO: Check if comparison with killer even necessary? killers are not captures!
                    if (   best.move != ttMove
                        && best.move != killers.first && best.move != killers.second)
                    {
                        return best.move;
                    }
                };

                return MOVE_NONE;

            }
            break;

        // Evasions
        // These are moves which escape the check
        case GENERATE_EVASIONS:

            {

                // Generate all evasions (both quiet and captures, also in quiescence search)
                // and score them with either mvv-lva or history values
                assert(board.checkers());
                
                ++phase;

                moves = generate_moves<EVASION, PSEUDO_LEGAL>(board, board.turn());
                score_evasions();

            }

        case EVASIONS:

            {
                
                ScoredMoveEntry best;
                // Pick the best evasion
                while ((best = moves.pick()).move != MOVE_NONE) {
                    moves.next();
                    if (best.move != ttMove) {
                        return best.move;
                    }
                };

                // Do not continue with any other stages; all moves evading check have already been searched here
                return MOVE_NONE;

            }

        // Captures in quiescence search
        case GENERATE_CAPTURES_QS:

            {
                ++phase;

                // Same as phase GENERATE_CAPTURES, only for quiescence search
                moves = generate_moves<CAPTURE, PSEUDO_LEGAL>(board, board.turn());
                score_captures();

            }

        case CAPTURES_QS:

            {

                ScoredMoveEntry best;
                while ((best = moves.pick()).move != MOVE_NONE) {
                    moves.next();
                    if (best.move != ttMove) {
                        return best.move;
                    }
                };

                return MOVE_NONE;

            }

        default:

            assert(false);

    }

    assert(false);
    return false;

}


// Picks the move in the list with the highest score and returns its index
ScoredMoveEntry ScoredMoveList::pick() {

    if (currentIndex == _size) {
        return { MOVE_NONE, 0 };
    }

    unsigned bestIndex = currentIndex;
    int bestScore = scores[bestIndex];

    for (unsigned i = currentIndex; i < _size; i++) {
        if (scores[i] > bestScore) {
            bestIndex = i;
            bestScore = scores[i];
        }
    }

    swap(currentIndex, bestIndex);

    return { moves[currentIndex], scores[currentIndex] };

}

void ScoredMoveList::swap(const unsigned index1, const unsigned index2) {

    MoveList::swap(index1, index2);
    std::iter_swap(scores.begin() + index1, scores.begin() + index2);
    
}

MovePickerPhase& operator++(MovePickerPhase& phase) {
    phase = static_cast<MovePickerPhase>(static_cast<unsigned>(phase) + 1);
    return phase;
}

MovePickerPhase operator++(MovePickerPhase& phase, int) {
    phase = static_cast<MovePickerPhase>(static_cast<unsigned>(phase) + 1);
    return phase;
}