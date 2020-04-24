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

#include "search.hpp"
#include "evaluate.hpp"
#include "hashkeys.hpp"
#include "uci.hpp"
#include "movepick.hpp"
#include "timeman.hpp"

// 2-dimensional array holding the number of plies to reduce the search
// given the current depth and the number of moves which have already
// been played.
static int LMRTable[MAX_DEPTH][MAX_MOVES];

// Initializes search parameters which are computed at execution time
void init_search() {

    for (int d = 1; d < MAX_DEPTH; d++) {
        for (int m = 1; m < MAX_MOVES; m++) {
            LMRTable[d][m] = 1 + log(d) * log(m) / 2;
        }
    }

}

// Merge two principal variations
void PvLine::merge(PvLine pv) {

    unsigned i;
    for (i = 0; i < pv.size; i++) {
        line[size + i] = pv.line[i];
    }
    size += i;

}

// Add a move to a principal variation
void PvLine::append(const Move move) {

    line[size] = move;
    ++size;

}

// Compare two principal variations
bool PvLine::compare(const PvLine& pv) const {

    // If the lenght is different, the variations cannot be the same
    if (pv.size > size) {
        return false;
    }

    for (unsigned int pcount = 0; pcount < pv.size; pcount++) {
        if (line[pcount] != pv.line[pcount]) return false;
    }
    return true;

}

// Reset a principal variation
void PvLine::clear() {

    size = 0;
    std::fill(line, line + MAX_DEPTH, MOVE_NONE);

}

// Resets history and counter move heuristics collected during
// other iterations of search
void clear_history(SearchInfo* info) {

    for (Color c = WHITE; c < BOTH; c++) {
        for (Piecetype pt = PAWN; pt < PIECE_NONE+1; pt++) {
            for (unsigned sq = 0; sq < 64; sq++) {
                info->history[c][pt][sq] = 0;
                info->counterMove[c][pt][sq] = MOVE_NONE;
            }
        }
    }

}

// Clear the killer moves found in previous iterations/runs of search
void clear_killers(SearchInfo* info) {

    for (int i = 0; i < MAX_DEPTH; i++) {
        info->killers[i][0] = MOVE_NONE;
        info->killers[i][1] = MOVE_NONE;
    }

}

// Check it the allocated time for the current search is up
static void check_finished(SearchInfo* info) {

    if (info->limitTime) {

        if (is_time_exceeded(info)) {
            info->stopped = true;
        }

    }

}

// Get the locations of the least valuable piece of a given set of attackers for a given color
// If no attacker was found, return SQUARE_NONE
unsigned Board::least_valuable_piece(uint64_t attackers, const Color color) const {

    for (Piecetype pt = PAWN; pt <= KING; pt++) {
        uint64_t subset = attackers & pieces(color, pt);
        if (subset) {
            return lsb_index(subset);
        }
    }

    return SQUARE_NONE;

}

// Static Exchange Evaluation
// The function takes a given move and returns the total material gain which can be obtained
// by moving the piece to the target square. The function finished once the target square is
// "quiet", meaning there are no more potential attackers to the square or one player has run
// out of attacking pieces.
int Board::see(const Move move) const {

    if (move_type(move) != NORMAL) {
        return 0;
    }

    unsigned fromSq = from_sq(move);
    unsigned toSq   = to_sq(move);

    Color color = !stm;

    uint64_t mayXray   = bbPieces[PAWN] | bbPieces[BISHOP] | bbPieces[ROOK] | bbPieces[QUEEN]; // pieces which may reveal a slider once removed from the board
    uint64_t occupied  = bbColors[BOTH];
    uint64_t attackers = sq_attackers(WHITE, toSq, occupied) | sq_attackers(BLACK, toSq, occupied); // all attackers to the target square

    unsigned attacker = fromSq;
    Piecetype victim = PIECE_NONE;

    // If the move is a capture; set the initial victim piece type
    if (is_ep(move)) {
        victim = PAWN;
    } else {
        if (pieceTypes[toSq] != PIECE_NONE) {
            victim = pieceTypes[toSq];
        }

    }

    int value = 0;

    // Start the Static Exchange Evaluation Loop
    do {

        // Flip the current color to play
        color = !color;

        // If it is our turn, add the victim material value to the balance, otherwise subtract it
        if (color == stm) {
            value += SeeMaterial[victim];
        } else {
            value -= SeeMaterial[victim];
        }

        // Set the new victim, which is the current attacker
        victim = pieceTypes[attacker];

        // Remove the attacker from potential attackers
        attackers ^= SQUARES[attacker];
        occupied  ^= SQUARES[attacker];

        // By moving the attacker, a new attacker could have been revealed
        if (SQUARES[attacker] & mayXray) {
            attackers |= slider_attackers(toSq, occupied) & occupied;
        }

        // Get the next attacker, which is the least valuable piece of the opposite color attacking the square
        attacker = least_valuable_piece(attackers, !color);

    } while (attacker != SQUARE_NONE && pieceTypes[attacker] != KING); // Stop the loop if a player has run out of attackers or the king is the final attacker

    return value;

}

// Update killer, counter move and history statistics for a quiet best move
static void update_quiet_stats(const Board& board, SearchInfo *info, const int plies, const int depth, const MoveList& quiets, const Move bestMove) {


    // Set the move as new killer move for the current ply if it not already is
    if (bestMove != info->killers[plies][0]) {
        info->killers[plies][1] = info->killers[plies][0];
        info->killers[plies][0] = bestMove;
    }

    // If the previous search depth was not a null move search, set the counter move
    if (info->currentMove[plies-1] != MOVE_NONE) {
        const unsigned prevSq = to_sq(info->currentMove[plies-1]);
        info->counterMove[board.owner(prevSq)][board.piecetype(prevSq)][prevSq] = bestMove;
    }

    // The history bonus should rise exponentially with depth
    int bonus = std::min(400, depth * depth);

    // Update the history table.
    // Increase the value of the best move found and decrease it for all other moves
    for (unsigned i = 0; i < quiets.size; i++) {
        Move move = quiets.moves[i];
        unsigned fromSq = from_sq(move);
        unsigned toSq = to_sq(move);
        Piecetype pt = board.piecetype(fromSq);

        int delta = (move == bestMove) ? bonus : -bonus;
        int score = info->history[board.turn()][pt][toSq];
        info->history[board.turn()][pt][toSq] += 32 * delta - score * std::abs(delta) / 512; // Formula for calculating new history score
    }

}

// The quiescent search function.
// This function uses the alpha-beta algorithm just like the search() method, however, it only
// searches captures and evasions until the position is "quiet", meaning there are no more captures
// or checks. This is important so the engine does not suffer from the horizon effect.
static int qsearch(int alpha, int beta, int depth, int plies, Board& board, SearchInfo *info) {

    assert(alpha >= -VALUE_INFINITE && beta <= VALUE_INFINITE && alpha < beta); // alpha and beta have to be within the given bounds; always alpha < beta!

    info->nodes++; // Increase the number of total nodes visited
    info->selectiveDepth = std::max(info->selectiveDepth, plies); // Set the current selective depth

    if ((info->nodes & 1023) == 1023) {
        check_finished(info);
    }

    // Check if the search has been stopped or the current position is a draw
    if (info->stopped || board.check_draw()) {
        return VALUE_DRAW;
    }

    const bool pvNode = (beta - alpha != 1); // Check if we are in a pv node (no zero window search)
    Move ttMove = MOVE_NONE;

    int bestValue, value, eval, deltaBase, deltaValue;
    bool ttHit;

    const int oldAlpha = alpha;
    const bool inCheck = board.checkers();
    const int ttDepth = (inCheck || depth >= 0) ? 0 : -1; // Set the depth for the transposition table entry (is constant because quiescent search depth is relative and would be invalid in further iterations)

    info->currentMove[plies] = MOVE_NONE;

    // Probe the Transposition Table
    TTEntry * entry = tTable.probe(board.hashkey(), ttHit);

    if (!pvNode && ttHit && entry->depth >= ttDepth) {

        const int ttValue = value_from_tt(entry->value, plies);
        ttMove = entry->bestMove;

        if (   ttValue != VALUE_NONE
            && ((entry->flag == BOUND_EXACT)
             || (entry->flag == BOUND_UPPER && ttValue <= alpha)
             || (entry->flag == BOUND_LOWER && ttValue >= beta)))
        {
            return ttValue;
        }

    }

    if (inCheck) {

        eval = VALUE_NONE;
        bestValue = deltaBase = -VALUE_INFINITE;

    } else {

        // Check if we can use the evaluation of the transposition table entry so we do not have
        // to recompute it
        bestValue = eval = (ttHit && entry->eval != VALUE_NONE ? entry->eval : evaluate(board));

        if (bestValue >= beta) {
            return bestValue;
        }

        if (bestValue > alpha) {
            alpha = bestValue;
        }

        deltaBase = bestValue + DeltaMargin;

    }

    int moveCount = 0;
    Move bestMove = MOVE_NONE;
    Move move = MOVE_NONE;

    MovePicker picker(board, info, plies, info->currentMove[plies-1], ttMove);

    while ( (move = picker.pick()) != MOVE_NONE ) {

        moveCount++;

        const bool givesCheck = board.gives_check(move);

        // Delta pruning (futility pruning in quiescent search)
        if (   !inCheck
            && !givesCheck
            && !board.is_dangerous_pawn_push(move))
        {

            deltaValue = deltaBase + Material[board.piecetype(to_sq(move))].eg;

            // If alpha is already above the material gain by DeltaMargin,
            // prune the move.
            if (deltaValue <= alpha) {
                bestValue = std::max(bestValue, deltaValue);
                continue;
            }

            // Also prune the move if the move is not gaining or losing
            // any material and is still below the best value + DeltaMargin
            if (deltaBase <= alpha && board.see(move) <= 0) {
                bestValue = std::max(bestValue, deltaBase);
                continue;
            }

        }

        // Prune moves with a negative static exchange evaluation
        if (!inCheck && board.see(move) < 0) {
            continue;
        }

        if (!board.is_legal(move)) {
            moveCount--;
            continue;
        }

        board.do_move(move);

        info->currentMove[plies] = move;

        // Recursive quiescent search. Flip alpha and beta
        value = -qsearch(-beta, -alpha, depth - 1, plies + 1, board, info);

        board.undo_move();

        // If the value found is better than the current best value, assign it as new best value
        if (value > bestValue) {
            bestValue = value;
            // If the value is above alpha, assign the move as new best move in the current node
            if (value > alpha) {
                bestMove = move;
                if (pvNode && value < beta) {
                    alpha = value; // Assign the value as new alpha
                } else {
                    // Beta cut-off if the value is above beta, which means this position will never be reached because the opposite player already has a better option at a higher depth
                    break;
                }
            }
        }

    }

    // If we are in check and there are no legal moves, return a mate value
    if (inCheck && moveCount == 0) {
        return -VALUE_MATE + plies;
    }

    // Store the value, evaluation and best move found in a transposition table entry
    tTable.store(board.hashkey(), ttDepth, value_to_tt(bestValue, plies), eval, bestMove, bestValue >= beta ? BOUND_LOWER : pvNode && bestValue > oldAlpha ? BOUND_EXACT : BOUND_UPPER);
    return bestValue;

}

// The main search function using an alpha-beta search algorithm. This is where the magic happens.
// The function takes alpha and beta as parameters, with alpha being the lowest value we can expect and beta the highest.
// Depth determines the number of plies we will look ahead, while plies represent the real number of moves actually played so far since depth can be increased/decreased dynamically during search
static int search(int alpha, int beta, int depth, int plies, bool cutNode, Board& board, SearchInfo *info, PvLine& pv, bool pruning, Move excluded = MOVE_NONE) {

    if ((info->nodes & 1023) == 1023) {
        check_finished(info);
    }

    // If we have gone this far in the search tree, do not look any further but descend into a quiescent search.
    // This means we will still look ahead until the position is "quiet"
    if (depth <= 0) {

        return qsearch(alpha, beta, 0, plies, board, info);

    } else {

        assert(alpha >= -VALUE_INFINITE && beta <= VALUE_INFINITE && alpha < beta); // alpha and beta have to be within the given bounds; always alpha < beta!
        assert(depth > 0 && depth <= MAX_DEPTH);

        info->nodes++; // Increase the total number of nodes visited
        info->selectiveDepth = std::max(info->selectiveDepth, plies); // Update the selective depth

        // Check if the search has been stopped or the current position is a draw
        if (info->stopped || board.check_draw()) {
            return VALUE_DRAW;
        }

        const bool rootNode = (plies == 0); // Check if we are in the root node (the first node of the search tree)
        const bool pvNode   = (beta - alpha != 1); // Check if we are in a pv node (no zero window search)

        PvLine newpv;
        MoveList quietMoves;
        bool ttHit = false;
        bool improving = false;
        unsigned moveCount = 0;
        int value, bestValue, eval;
        int ttValue = VALUE_NONE;
        value = bestValue = -VALUE_INFINITE;
        eval = info->eval[plies] = VALUE_NONE;

        info->currentMove[plies] = MOVE_NONE;
        info->killers[plies + 1][0] = info->killers[plies + 1][1] = MOVE_NONE;

        TTEntry * entry;
        Move ttMove = MOVE_NONE;
        Move bestMove = MOVE_NONE;

        if (excluded == MOVE_NONE) { // Don't do transposition table probing during a singular search extension

            entry = tTable.probe(board.hashkey(), ttHit);

            if (ttHit) {

                ttMove = entry->bestMove;
                ttValue = value_from_tt(entry->value, plies);

                if (!pvNode && entry->depth >= depth) {
                    if ((entry->flag == BOUND_EXACT)
                     || (entry->flag == BOUND_UPPER && ttValue <= alpha)
                     || (entry->flag == BOUND_LOWER && ttValue >= beta)) {
                        return ttValue;
                    }
                }

            }

        }

        const bool inCheck = board.checkers();

        // Static Evaluation of the current position
        if (!inCheck) {

            // Use the evaluation of the transposition table entry if available; otherwise calculate it and store it as a new entry
            if (ttHit && entry->eval != VALUE_NONE) {
                eval = info->eval[plies] = entry->eval;
            } else {
                eval = info->eval[plies] = evaluate(board);
                tTable.store(board.hashkey(), DEPTH_NONE, VALUE_NONE, eval, MOVE_NONE, BOUND_NONE);
            }

        }

        if (pruning) {

            // Razoring
            // If we are near the leaf nodes, and our static evaluation is less or equal to alpha by a margin,
            // we can assume that moves in this position are not good enough to beat alpha. We go into
            // quiescent search immediately to verify this.
            if (   !rootNode
                && depth == 1
                && eval <= alpha - RazorMargin)
            {
                return qsearch(alpha, beta, 0, plies, board, info);
            }

            // Null move pruning
            // If we can give the opponent an extra move py passing this turn, the chance that the position
            // is already way too good for us is very high,and we do not need to search it any further.
            // We do a reduced search with a null window here to verify that we fail high. The reduction is
            // dynamic
            if (   !pvNode
                && depth >= 2
                && !inCheck
                && board.minors_or_majors(board.turn())
                && eval >= beta)
            {
                board.do_nullmove();
                value = -search(-beta, -beta + 1, depth - (2 + (32 * depth + std::min(eval - beta, 512)) / 128), plies + 1, !cutNode, board, info, newpv, false);
                board.undo_nullmove();

                if (value >= beta && std::abs(value) < VALUE_MATE_MAX) {
                    return beta;
                }
            }

        }

        // Check if the evaluation is improving since our last turn
        improving = plies >= 2 && (eval >= info->eval[plies - 2] || info->eval[plies - 2] == VALUE_NONE);

        // Internal Iterative Deepening
        // If we are in a pv node and do not have a best move from the transposition table,
        // we do a reduced depth search to fill up the transposition table with entries to
        // obtain a transposition table move for the current position to further imporove
        // move ordering
        if (   pvNode
            && !inCheck
            && ttMove == MOVE_NONE
            && depth >= 6)
        {

            value = search(alpha, beta, depth - 2, plies + 1, cutNode, board, info, newpv, pruning);

            entry = tTable.probe(board.hashkey(), ttHit);

            if (ttHit) {
                ttMove = entry->bestMove;
            }
        }

        // Initialize the move picker
        MovePicker picker(board, info, plies, ttMove);

        Move move;

        while ( (move = picker.pick() ) != MOVE_NONE) {

            // Skip excluded moves (from singular search)
            if (move == excluded) {
                continue;
            }

            // Flag the current move
            bool capture = board.is_capture(move);
            bool givesCheck = board.gives_check(move);
            bool promotion = is_promotion(move);

            // Add quiet moves to a list
            if (!capture && !promotion) {
                quietMoves.append(move);
            }

            // Quiet Move Pruning
            // A move is considered quiet if it does not capture a piece, gives check or is a promtion.
            if (   !capture
                && !givesCheck
                && !promotion)
            {
                // Futility Pruning
                // If the static evaluation is below alpha by a given margin and we are not in check,
                // prune the move.
                if (   !pvNode
                    && !inCheck
                    && moveCount > 0
                    && depth <= 5
                    && eval + FutilityMargin[depth] <= alpha)
                {
                    continue;
                }
            }

            moveCount++;

            int reductions = 0;
            int extensions = 0;

            // Extensions

            // Singular Extension Search
            // If the current move is the transposition table move, and the move caused
            // a fail high in a previous iteration, the move should probably be extended.
            // To verify this, we do a reduced search with a null window
            if (   depth >= 8
                && move == ttMove
                && excluded == MOVE_NONE // No recursive singular search
                && !rootNode
                && ttValue != VALUE_NONE
                && entry->flag == BOUND_LOWER
                && entry->depth >= depth - 3
                && board.is_legal(move))
            {
                int rbeta = std::max(ttValue - 2 * depth, -VALUE_MATE);
                value = search(rbeta - 1, rbeta, depth / 2, plies + 1, cutNode, board, info, newpv, false, move);
                // All other moves failed low, so the move is singular
                if (value < rbeta) {
                    extensions = 1;
                }
            } else {
                // Check Extension
                if (inCheck && board.see(move) >= 0) {
                    extensions = 1;
                }
            }

            // Finally, check if the move is even legal.
            // Since this is quite costly, we delay it until we really have to check,
            // which is right before doing the move
            if (!board.is_legal(move)) {
                moveCount--;
                continue;
            }

            // Play the move on the board
            board.do_move(move);

            info->currentMove[plies] = move;

            newpv.size = 0;

            // Late Move Reductions
            // If a move is relatively far back in the move list, we can consider to do a reduced depth search
            // because we usually have a relatively good move ordering. Certain circumstances can increase or
            // decrease the amount of reduction. We only reduce quiet moves. We also do not reduce near the leaf nodes
            // since this is quite risky because we might miss something.
            if (moveCount > 1) {
                if (   !capture
                    && !givesCheck
                    && !promotion
                    && depth >= 3)
                {

                    // Base reduction based on current depth and move count
                    reductions = LMRTable[depth][moveCount];

                    // Decrease reduction for pv nodes since we want a relatively precise value for these types of nodes
                    reductions -= pvNode;

                    // Increase the reduction for cut nodes since the actual value is not that important
                    reductions += cutNode;

                    // Decrease reduction for killer and counter moves since they are usually good moves and cause a quick fail high which reduces the tree size
                    reductions -= (move == info->killers[plies][0] || move == info->killers[plies][1] || move == picker.counterMove);

                    // Decrease reduction if we are in check since the position might be very dynamic
                    reductions -= inCheck;

                    // Decrease the reduction based on the history score. If the move has proven to be quite good in previous iterations,
                    // we should not reduce the search depth
                    reductions -= std::min(1, info->history[!board.turn()][board.piecetype(to_sq(move))][to_sq(move)] / 512);

                    // Do not reduce more than depth - 2, also do not extend
                    reductions = std::max(0, std::min(reductions, depth - 2));

                }

                // Principal Variation Search
                // We do a null window search here because we only want to know if the current move can beat alpha.
                value = -search(-alpha - 1, -alpha, depth - 1 + extensions - reductions, plies + 1, true, board, info, newpv, pruning);
                // Do a full depth search if the value did beat alpha since we might have missed something
                if (value > alpha && reductions) {
                    value = -search(-alpha - 1, -alpha, depth - 1 + extensions, plies + 1, !cutNode, board, info, newpv, pruning);
                }
                // If the full depth searched beat alpha again and is within alpha and beta, do a full window search to prove it
                if (value > alpha && value < beta) {
                    value = -search(-beta, -alpha, depth - 1 + extensions, plies + 1, false, board, info, newpv, pruning);
                }

            } else {

                // If this is the first move to be searched in this node, search it with a full window and do not reduce it,
                // since usually the first move is already the best
                value = -search(-beta, -alpha, depth - 1 + extensions, plies + 1, (pvNode ? false : !cutNode), board, info, newpv, pruning);

            }

            // Undo the move.
            board.undo_move();

            // Abort if the search has been stopped
            if (info->stopped) {
                return VALUE_DRAW;
            }

            // If our last search increased found a higher value, assign it as the best value
            if (value > bestValue) {
                bestValue = value;
                // If the value is above alpha, assign it as the new alpha.
                // Also add the move to the principal variation.
                if (value > alpha) {
                    alpha = value;
                    bestMove = move;
                    pv.size = 0;
                    pv.append(move);
                    pv.merge(newpv);
                    // If the value is above beta, we can expect this position will never occur because the opponent
                    // will probably avoid it because he already has a better option at a higher depth. We can stop searching
                    // this node.
                    if (value >= beta) {
                        break; // Beta cut-off
                    }

                }

            }

        }

        // If there are no legal moves, check if we are checkmate or if the position is drawn
        if (moveCount == 0) {
            return excluded != MOVE_NONE ? alpha : inCheck ? -VALUE_MATE + plies : VALUE_DRAW;
        }

        // If we failed high, and the move is quiet, update the quiet move stats.
        if (bestValue >= beta && !is_promotion(bestMove) && !board.is_capture(bestMove)) {
            update_quiet_stats(board, info, plies, depth, quietMoves, bestMove);
        }

        if (excluded == MOVE_NONE) { // Do not store if we are in a singular search
            // Store the value, the best move and the bound in the transposition table
            tTable.store(board.hashkey(), depth, value_to_tt(bestValue, plies), eval, bestMove, bestValue >= beta ? BOUND_LOWER : pvNode && bestMove != MOVE_NONE ? BOUND_EXACT : BOUND_UPPER);
        }
        return bestValue;

    }

}

// Find the best move for the given position.
// The search can be constrained by depth or time or nodes.
const SearchStats go(Board& board, const SearchLimits& limits) {

    SearchStats stats;
    SearchInfo info;

    PvLine pv;
    pv.clear();
    Move bestMove = MOVE_NONE;
    int value = 0;

    // Initialize the time management
    info.start = Clock::now();
    init_time_management(limits, &info);

    clear_history(&info);
    clear_killers(&info);

    // Iterative Deepening
    // We do not search to a fixed depth from the start, but rather increase it with every
    // iteration. This might seem to be unproductive, but however because of the searches
    // at lower depths we fill up the transposition table, history table...
    // This enables us to search higher depths much quicker and also enables us to dynamically
    // stop the search if we are low on time while still having a move to play in the position
    for (unsigned depth = 1; depth < limits.depth; depth++) {

        pv.size = 0;

        board.reset_plies();

        info.depth = depth;
        info.selectiveDepth = 0;

        // Aspiration Windows
        // We do not have to do a search with a full window at a certain point,
        // since we can expect the values returned by search to be within a certain
        // window. If we do fail high/low though, we will increase the window and search
        // again.
        if (depth > 5) {

            int window = 25 - std::min(depth / 3, 10u) + std::abs(value) / 25;
            int alpha = value - window;
            int beta = value + window;

            while (true) {

                value = search(alpha, beta, depth, 0, false, board, &info, pv, true);

                if (!info.stopped) {
                    if (value <= alpha) {
                        beta = (alpha + beta) / 2;
                        alpha = value - window;
                    } else if (value >= beta) {
                        beta = value + window;
                    } else {
                        break; // Quit the aspiration window loop and continue with the next iteration
                    }

                    // Increase the window size if we have failed high/low
                    window += window / 4;

                    pv.clear();
                } else {
                    break;
                }

            }

        } else {
            value = search(-VALUE_INFINITE, VALUE_INFINITE, depth, 0, false, board, &info, pv, true);
        }

        info.value[depth] = value;
        long long duration = get_time_elapsed(info.start);

        // Print the information returned from the previous iteration to the console
        // Update the time management and decide if we should do another iteration or
        // stop and report the best move
        if (!info.stopped) {
            // Drawn/mate positions return an empty pv
            if (pv.size > 0) {

                bestMove = pv.line[0];
                info.bestMove[depth] = bestMove;
                send_info(&info, pv, duration);

            }  else {
                send_info(&info, pv, duration);
                break;
            }

            update_time_managemement(&info);

            if (info.limitTime) {
                if (should_stop(info)) {
                    break;
                }
            }
        } else {
            // If the search has been aborted, break the search loop
            break;
        }

    }

    // Print the best move found to the console
    send_bestmove(bestMove);

    stats.totalNodes = info.nodes;

    return stats;

}
