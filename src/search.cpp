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

#include <fstream>

#include "search.hpp"
#include "thread.hpp"
#include "evaluate.hpp"
#include "hashkeys.hpp"
#include "uci.hpp"
#include "movepick.hpp"
#include "timeman.hpp"

// 2-dimensional array holding the number of plies to reduce the search
// given the current depth and the number of moves which have already
// been played.
static int LMRTable[DEPTH_MAX][MOVES_MAX_COUNT];

// Initializes search parameters which are computed at execution time
void init_search() {

    for (int d = 1; d < DEPTH_MAX; d++) {
        for (int m = 1; m < MOVES_MAX_COUNT; m++) {
            LMRTable[d][m] = 1 + log(d) * log(m) / 2;
        }
    }

}

// Update a principal variations
void PrincipalVariation::update(const Move bestMove, const PrincipalVariation& pv) {

    size = 0;
    line[size] = bestMove;
    ++size;

    unsigned i;
    for (i = 0; i < pv.size; i++) {
        line[size + i] = pv.line[i];
    }
    size += i;

}

void SearchInfo::reset() {

    hashTableHits = nodes = depth = selectiveDepth = pvStability = multiPv = 0;
    idealTime = maxTime = 0;

    bestMove.fill(MOVE_NONE);
    currentMove.fill(MOVE_NONE);
    multiPvMoves.fill(MOVE_NONE);
    eval.fill(0);
    value.fill(0);

}

// Resets history and counter move heuristics collected during
// other iterations of search
void Thread::clear_history() {

    for (Color c = WHITE; c < COLOR_COUNT; c++) {
        for (Piecetype pt = PAWN; pt < PIECETYPE_COUNT+1; pt++) {
            for (unsigned sq = 0; sq < 64; sq++) {
                history[c][pt][sq] = 0;
                counterMove[c][pt][sq] = MOVE_NONE;
            }
        }
    }

}

// Clear the killer moves found in previous iterations/runs of search
void Thread::clear_killers() {

    for (int i = 0; i < DEPTH_MAX; i++) {
        killers[i][0] = MOVE_NONE;
        killers[i][1] = MOVE_NONE;
    }

}

static Value get_draw_value(Depth depth, SearchInfo* info) {

    return depth > 3 ? 1 - static_cast<Value>(info->nodes & 2) : VALUE_DRAW;

}

static Value get_mated_value(Depth plies) {

    return -VALUE_MATE + plies;

}

static Value get_mate_value(Depth plies) {

    return VALUE_MATE - plies;

}

// Check it the allocated time for the current search is up
static void check_finished(SearchInfo* info) {

    if (   ((info->limits.time || info->limits.moveTime) && is_time_exceeded(info))
        || (info->limits.nodes && Threads.get_nodes() >= info->limits.nodes))
    {
        Threads.stop_searching();
    }

}

// Get the locations of the least valuable piece of a given set of attackers for a given color
// If no attacker was found, return SQUARE_NONE
unsigned Board::least_valuable_piece(Bitboard attackers, const Color color) const {

    for (Piecetype pt = PAWN; pt <= KING; pt++) {
        Bitboard subset = attackers & pieces(color, pt);
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
Value Board::see(const Move move) const {

    if (move_type(move) != NORMAL) {
        return 0;
    }

    Square fromSq = from_sq(move);
    Square toSq   = to_sq(move);

    Color color = !stm;

    Bitboard mayXray   = bbPieces[PAWN] | bbPieces[BISHOP] | bbPieces[ROOK] | bbPieces[QUEEN]; // pieces which may reveal a slider once removed from the board
    Bitboard occupied  = bbColors[BOTH];
    Bitboard attackers = sq_attackers(WHITE, toSq, occupied) | sq_attackers(BLACK, toSq, occupied); // all attackers to the target square

    Square attacker = fromSq;
    Piecetype victim = PIECE_NONE;

    // If the move is a capture; set the initial victim piece type
    if (is_ep(move)) {
        victim = PAWN;
    } else {
        if (pieceTypes[toSq] != PIECE_NONE) {
            victim = pieceTypes[toSq];
        }

    }

    Value value = 0;

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
static void update_quiet_stats(Thread *thread, const Board& board, SearchInfo *info, const Depth plies, const Depth depth, const MoveList& quiets, const Move bestMove) {

    // Set the move as new killer move for the current ply if it not already is
    if (bestMove != thread->killers[plies][0]) {
        thread->killers[plies][1] = thread->killers[plies][0];
        thread->killers[plies][0] = bestMove;
    }

    // If the previous search depth was not a null move search, set the counter move
    if (info->currentMove[plies-1] != MOVE_NONE) {
        const Square prevSq = to_sq(info->currentMove[plies-1]);
        thread->counterMove[board.owner(prevSq)][board.piecetype(prevSq)][prevSq] = bestMove;
    }

    // The history bonus should rise exponentially with depth
    int bonus = std::min(400, depth * depth);

    // Update the history table.
    // Increase the value of the best move found and decrease it for all other moves
    for (unsigned i = 0; i < quiets.size; i++) {
        Move move = quiets.moves[i];
        Square fromSq = from_sq(move);
        Square toSq = to_sq(move);
        Piecetype pt = board.piecetype(fromSq);

        int delta = (move == bestMove) ? bonus : -bonus;
        int score = thread->history[board.turn()][pt][toSq];
        thread->history[board.turn()][pt][toSq] += 32 * delta - score * std::abs(delta) / 512; // Formula for calculating new history score
    }

}

static bool multipv_move_played(const SearchInfo* info, const Move move) {

    for (unsigned moveIndex = 0; moveIndex < info->multiPv; moveIndex++) {
        if (info->multiPvMoves[moveIndex] == move) {
            return true;
        }
    }

    return false;

}

// The quiescent search function.
// This function uses the alpha-beta algorithm just like the search() method, however, it only
// searches captures and evasions until the position is "quiet", meaning there are no more captures
// or checks. This is important so the engine does not suffer from the horizon effect.
static Value qsearch(Value alpha, Value beta, Depth depth, Depth plies, Board& board, SearchInfo *info) {

    assert(alpha >= -VALUE_INFINITE && beta <= VALUE_INFINITE && alpha < beta); // alpha and beta have to be within the given bounds; always alpha < beta!

    info->nodes++; // Increase the number of total nodes visited
    info->selectiveDepth = std::max(info->selectiveDepth, plies); // Set the current selective depth

    if (info->isMainThread && (info->nodes & 1023) == 1023) {
        check_finished(info);
    }

    const bool inCheck = board.checkers();

    // Check if the search has been stopped or the current position is a draw
    if (Threads.has_stopped() || board.check_draw()) {
        return VALUE_DRAW;
    }

    if (plies >= DEPTH_MAX) {
        return inCheck ? VALUE_DRAW : evaluate(board, info->threadIndex);
    }

    const bool pvNode = (beta - alpha != 1); // Check if we are in a pv node (no zero window search)

    Thread *thread = Threads.get_thread(info->threadIndex);
    Value bestValue, value, eval, deltaBase, deltaValue;
    bool ttHit = false;

    const Value oldAlpha = alpha;
    const Depth ttDepth  = (inCheck || depth >= 0) ? 0 : -1; // Set the depth for the transposition table entry (is constant because quiescent search depth is relative and would be invalid in further iterations)

    info->currentMove[plies] = MOVE_NONE;

    // Probe the Transposition Table
    TTEntry * entry = TTable.probe(board.hashkey(), ttHit);
    Move ttMove = MOVE_NONE;

    if (   !pvNode
        && ttHit
        && entry->depth() >= ttDepth) {

        Value ttValue = value_from_tt(entry->value(), plies);
        ttMove = entry->move();

        if (   ttValue != VALUE_NONE
            && ((entry->bound() == BOUND_EXACT)
             || (entry->bound() == BOUND_UPPER && ttValue <= alpha)
             || (entry->bound() == BOUND_LOWER && ttValue >= beta)))
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
        if (ttHit) {
            eval = entry->eval();
            if (eval == VALUE_NONE) {
                eval = evaluate(board, info->threadIndex);
            }
        } else {
            eval = evaluate(board, info->threadIndex);
        }

        info->eval[plies] = bestValue = eval;

        if (bestValue >= beta) {
            return bestValue;
        }

        if (pvNode && bestValue > alpha) {
            alpha = bestValue;
        }

        deltaBase = bestValue + DeltaMargin;

    }

    unsigned movesCount;
    movesCount = 0;
    Move bestMove = MOVE_NONE;
    Move move = MOVE_NONE;

    MovePicker picker(thread, board, info, plies, info->currentMove[plies-1], ttMove);

    while ( (move = picker.pick()) != MOVE_NONE ) {

        if (!board.is_legal(move)) {
            continue;
        }

        movesCount++;

        const bool givesCheck = board.gives_check(move);

        // Delta pruning (futility pruning in quiescent search)
        if (   !inCheck
            && !givesCheck
            //&& !is_ep(move)
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
    if (inCheck && movesCount == 0) {
        return get_mated_value(plies);
    }

    // Store the value, evaluation and best move found in a transposition table entry
    TTable.store(board.hashkey(), ttDepth, value_to_tt(bestValue, plies), eval, bestMove, bestValue >= beta ? BOUND_LOWER : pvNode && bestValue > oldAlpha ? BOUND_EXACT : BOUND_UPPER);
    
    assert(bestValue > -VALUE_INFINITE && bestValue < VALUE_INFINITE);
    
    return bestValue;

}

// The main search function using an alpha-beta search algorithm. This is where the magic happens.
// The function takes alpha and beta as parameters, with alpha being the lowest value we can expect and beta the highest.
// Depth determines the number of plies we will look ahead, while plies represent the real number of moves actually played so far since depth can be increased/decreased dynamically during search
static Value search(Value alpha, Value beta, Depth depth, Depth plies, bool cutNode, Board& board, SearchInfo *info, PrincipalVariation& pv, bool pruning, Move excluded = MOVE_NONE) {

    if (info->isMainThread && (info->nodes & 1023) == 1023) {
        check_finished(info);
    }

    // If we have gone this far in the search tree, do not look any further but descend into a quiescent search.
    // This means we will still look ahead until the position is "quiet"
    if (depth <= 0) {
        return qsearch(alpha, beta, 0, plies, board, info);
    }

    assert(alpha >= -VALUE_INFINITE && beta <= VALUE_INFINITE && alpha < beta); // alpha and beta have to be within the given bounds; always alpha < beta!
    assert(depth > 0 && depth <= DEPTH_MAX);

    info->nodes++; // Increase the total number of nodes visited
    info->selectiveDepth = std::max(info->selectiveDepth, plies + 1); // Update the selective depth

    const bool rootNode = (plies == 0); // Check if we are in the root node (the first node of the search tree)
    const bool pvNode   = (beta - alpha != 1); // Check if we are in a pv node (no zero window search)

    assert(!(pvNode && cutNode));

    const bool inCheck = board.checkers();

    if (!rootNode) {
        // Check if the search has been stopped or the current position is a draw
        if (Threads.has_stopped()) {
            return VALUE_DRAW;
        }
        
        // Checks for draw by 50-move rule or 3-fold repetition
        if (board.check_draw()) {
            return VALUE_DRAW; //TODO: This does not work yet: get_draw_value(depth, info);
        }

        if (plies >= DEPTH_MAX) {
            return inCheck ? VALUE_DRAW : evaluate(board, info->threadIndex);
        }

        // Mate Distance Pruning
        alpha = std::max(get_mated_value(plies), alpha);
        beta  = std::min(get_mate_value(plies + 1), beta);
        if (alpha >= beta) {
            return alpha;
        }

    }
    

    Thread *thread = Threads.get_thread(info->threadIndex);
    PrincipalVariation newPv;
    MoveList quietMoves;
    bool ttHit = false;
    bool improving = false;
    unsigned movesCount = 0;
    Value value, bestValue, ttValue, eval;
    value = bestValue = -VALUE_INFINITE;
    ttValue = eval = info->eval[plies] = VALUE_NONE;

    info->currentMove[plies] = MOVE_NONE;
    thread->killers[plies + 1][0] = thread->killers[plies + 1][1] = MOVE_NONE;

    TTEntry * entry;
    Move ttMove = MOVE_NONE;
    Move bestMove = MOVE_NONE;

    if (excluded == MOVE_NONE) { // Don't do transposition table probing during a singular search extension

        entry = TTable.probe(board.hashkey(), ttHit);

        if (ttHit) {

            ttMove = entry->move();
            ttValue = value_from_tt(entry->value(), plies);

            if (!pvNode && entry->depth() >= depth) {
                if ((entry->bound() == BOUND_EXACT)
                    || (entry->bound() == BOUND_UPPER && ttValue <= alpha)
                    || (entry->bound() == BOUND_LOWER && ttValue >= beta)) {
                    return ttValue;
                }
            }

        }

    }

    // Static Evaluation of the current position
    if (!inCheck) {

        // Use the evaluation of the transposition table entry if available; otherwise calculate it and store it as a new entry
        if (ttHit) {
            eval = entry->eval();
            if (eval == VALUE_NONE) {
                eval = evaluate(board, info->threadIndex);
            }
            
        } else {
            eval = evaluate(board, info->threadIndex);
            TTable.store(board.hashkey(), DEPTH_NONE, VALUE_NONE, eval, MOVE_NONE, BOUND_NONE);
        }

        info->eval[plies] = eval;

        // Check if the evaluation is improving since our last turn
        improving = plies >= 2 && (eval >= info->eval[plies - 2] || info->eval[plies - 2] == VALUE_NONE);

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
            && board.minors_and_majors(board.turn())
            && eval >= beta)
        {
            board.do_nullmove();
            value = -search(-beta, -beta + 1, depth - (2 + (32 * depth + std::min(eval - beta, 512)) / 128), plies + 1, !cutNode, board, info, newPv, false);
            board.undo_nullmove();

            if (value >= beta) {

                // We cannot trust mate values from null move search
                if (value >= VALUE_MATE_MAX) {
                    value = beta;
                }

                // Only return if there is no mate at higher depth
                if (std::abs(beta) < VALUE_MATE_MAX) {
                    return value;
                }

            }
        }

    }

    
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

        value = search(alpha, beta, depth - 2, plies + 1, cutNode, board, info, newPv, pruning);

        entry = TTable.probe(board.hashkey(), ttHit);

        if (ttHit) {
            ttMove = entry->move();
        }
    }

    // Initialize the move picker
    MovePicker picker(thread, board, info, plies, ttMove);

    Move move;
    Depth newDepth;

    while ( (move = picker.pick() ) != MOVE_NONE) {

        // Skip excluded moves (from singular search)
        if (move == excluded) continue;
        if (rootNode && multipv_move_played(info, move)) continue;
        
        if (!board.is_legal(move)) {
            continue;
        }

        movesCount++;

        // Flag the current move
        bool capture    = board.is_capture(move);
        bool givesCheck = board.gives_check(move);
        bool promotion  = is_promotion(move);
        bool quiet      = !capture && !promotion;

        // Add quiet moves to a list
        if (quiet) {
            quietMoves.append(move);
        }

        // Quiet Move Pruning
        // A move is considered quiet if it does not capture a piece, gives check or is a promotion.
        if (   !capture
            && !givesCheck
            && !promotion)
        {
            // Futility Pruning
            // If the static evaluation is below alpha by a given margin and we are not in check,
            // prune the move.
            if (   !pvNode
                && !inCheck
                && movesCount > 1
                && depth <= 5
                && eval + FutilityMargin[depth] <= alpha)
            {
                continue;
            }
        }

        newDepth = depth - 1;

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
            && entry->bound() == BOUND_LOWER
            && entry->depth() >= depth - 3
            && board.is_legal(move))
        {
            Value rbeta = std::max(ttValue - 2 * depth, -VALUE_MATE);
            value = search(rbeta - 1, rbeta, depth / 2, plies + 1, cutNode, board, info, newPv, false, move);
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

        newDepth += extensions;

        // Play the move on the board
        board.do_move(move);

        info->currentMove[plies] = move;

        newPv.reset();

        // Report the current move searched at main thread after a fixed amount of time
        if (   rootNode
            && info->isMainThread
            && get_time_elapsed(info->start) > 5000)
        {
            send_currmove(move, movesCount);
        }

        // Late Move Reductions
        // If a move is relatively far back in the move list, we can consider to do a reduced depth search
        // because we usually have a relatively good move ordering. Certain circumstances can increase or
        // decrease the amount of reduction. We only reduce quiet moves. We also do not reduce near the leaf nodes
        // since this is quite risky because we might miss something.
        if (   movesCount > 1
            && depth >= 3
            && quiet)
        {

            // Base reduction based on current depth and move count
            reductions = LMRTable[depth][movesCount];

            // Decrease reduction for pv nodes since we want a relatively precise value for these types of nodes
            reductions -= pvNode;

            // Increase the reduction for cut nodes since the actual value is not that important
            reductions += cutNode;

            // Decrease reduction for killer and counter moves since they are usually good moves and cause a quick fail high which reduces the tree size
            reductions -= (move == thread->killers[plies][0] || move == thread->killers[plies][1] || move == picker.counterMove);

            // Decrease reduction if we are in check since the position might be very dynamic
            reductions -= inCheck;

            // Decrease the reduction based on the history score. If the move has proven to be quite good in previous iterations,
            // we should not reduce the search depth
            reductions -= std::min(1, thread->history[!board.turn()][board.piecetype(to_sq(move))][to_sq(move)] / 512);

            // Do not reduce more than depth - 2, also do not extend
            reductions = std::max(0, std::min(reductions, depth - 2));

        }

        // Principal Variation Search
        // We do a null window search here because we only want to know if the current move can beat alpha.
        if (reductions) {
            value = -search(-alpha - 1, -alpha, newDepth - reductions, plies + 1, true, board, info, newPv, pruning);
        }

        // Do a full depth search if the value did beat alpha since we might have missed something
        if ((reductions && value > alpha) || (!reductions && (!pvNode || movesCount > 1))) {
            value = -search(-alpha - 1, -alpha, newDepth, plies + 1, !cutNode, board, info, newPv, pruning);
        }

        if (pvNode && (movesCount == 1 || (value > alpha && (rootNode || value < beta)))) {
            value = -search(-beta, -alpha, newDepth, plies + 1, false, board, info, newPv, pruning);
        }

        // Undo the move.
        board.undo_move();

        // Abort if the search has been stopped
        if (Threads.has_stopped()) {
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
                pv.update(bestMove, newPv);
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
    if (movesCount == 0) {
        if (excluded != MOVE_NONE) {
            return alpha;
        }
        if (inCheck) {
            return get_mated_value(plies);
        } else {
            return VALUE_DRAW;
        }
    }

    // If we failed high, and the move is quiet, update the quiet move stats.
    if (bestValue >= beta && !is_promotion(bestMove) && !board.is_capture(bestMove)) {
        update_quiet_stats(thread, board, info, plies, depth, quietMoves, bestMove);
    }

    // Store the depth, value, evaluation, best move and bound in the transposition table
    // Do not store if we are in a singular search or if we would overwrite an entry from
    // the first move we played in multiPV mode
    if (excluded == MOVE_NONE && !(rootNode && info->multiPv > 0)) {
        TTable.store(board.hashkey(), depth, value_to_tt(bestValue, plies), eval, bestMove, bestValue >= beta ? BOUND_LOWER : pvNode && bestMove != MOVE_NONE ? BOUND_EXACT : BOUND_UPPER);
    }

    assert(bestValue > -VALUE_INFINITE && bestValue < VALUE_INFINITE);

    return bestValue;

}

void Thread::search() {

    // Initialize the time management
    info.start = Clock::now();
    init_time_management(&info);

    const bool isMainThread = get_index() == 0;

    PrincipalVariation pv;
    Move bestMove = MOVE_NONE;
    Value value, alpha, beta, delta;
    value = 0;
    alpha = -VALUE_INFINITE;
    beta  = VALUE_INFINITE;

    info.threadIndex  = get_index();
    info.isMainThread = isMainThread;

    // Adjust multiPv to maximum number of legal moves in root position
    MoveList rootMoves  = gen_legals(board, gen_all(board, board.turn()));
    info.limits.multiPv = std::min(info.limits.multiPv, rootMoves.size);

    // Iterative Deepening
    // We do not search to a fixed depth from the start, but rather increase it with every
    // iteration. This might seem to be unproductive, but however because of the searches
    // at lower depths we fill up the transposition table, history table...
    // This enables us to search higher depths much quicker and also enables us to dynamically
    // stop the search if we are low on time while still having a move to play in the position
    for (Depth depth = 1; depth <= info.limits.depth && !Threads.has_stopped(); depth++) {

        info.depth = depth;

        // Multiple Prinicipal Variations
        // We search as many principal variations as specifified by the user
        // A variation always starts with a different root move, the best variation
        // will be first, the second best second, and so on
        for (unsigned multiPv = 0; multiPv < info.limits.multiPv && !Threads.has_stopped(); multiPv++) {

            board.reset_plies();

            info.selectiveDepth = 0;
            info.multiPv = multiPv;

            delta = 25;

            // Aspiration Windows
            // We do not have to do a search with a full window at a certain point,
            // since we can expect the values returned by search to be within a certain
            // window. If we do fail high/low though, we will increase the window and search
            // again.
            if (depth > 5) {

                alpha = std::max(value - delta, -VALUE_INFINITE);
                beta  = std::min(value + delta,  VALUE_INFINITE);

            }

            while (true) {

                pv.reset();

                value = ::search(alpha, beta, depth, 0, false, board, &info, pv, true);

                if (Threads.has_stopped()) {
                    break;
                }

                if (   isMainThread
                    && info.limits.multiPv == 1
                    && (value <= alpha || value >= beta)
                    && get_time_elapsed(info.start) > 3000)
                {
                    send_pv(info, value, pv, Threads.get_nodes(), alpha, beta);
                }

                if (value <= alpha) {
                    beta  = (alpha + beta) / 2;
                    alpha = std::max(value - delta, -VALUE_INFINITE);
                } else if (value >= beta) {
                    beta = std::min(value + delta, VALUE_INFINITE);
                } else {
                    break; // Quit the aspiration window loop and continue with the next iteration
                }

                // Increase the window size if we have failed high/low
                delta += delta / 4;

            }

            info.value[depth] = value;

            // On the main thread, update time management and decide if we 
            // should do another iteration or stop searching and report the best move
            if (isMainThread) {

                // Send the principal variation
                // Do not report incomplete searches
                if (!Threads.has_stopped()) {
                    send_pv(info, value, pv, Threads.get_nodes(), alpha, beta);
                }

                // Drawn/mate positions return an empty pv
                if (pv.length() > 0) {
                    info.multiPvMoves[multiPv] = pv.best();
                    if (multiPv == 0) {
                        bestMove = info.bestMove[depth] = info.multiPvMoves[0];
                    }
                }

                update_time_management(&info);

                if (info.limits.time) {
                    if (should_stop(info)) {
                        Threads.stop_searching();
                    }
                }

            }

        }

    }

    if (isMainThread) {
        // Signal all other threads to stop searching
        Threads.stop_searching();
        // Print the best move found to the console
        send_bestmove(bestMove);
    }

}
