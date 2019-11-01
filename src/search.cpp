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

#include "search.hpp"
#include "evaluate.hpp"
#include "hashkeys.hpp"
#include "uci.hpp"
#include "movepick.hpp"
#include "timeman.hpp"

static int LMRTable[MAX_DEPTH][MAX_MOVES];

void init_search() {

    for (int d = 1; d < MAX_DEPTH; d++) {
        for (int m = 1; m < MAX_MOVES; m++) {
            LMRTable[d][m] = 1 + log(d) * log(m) / 2;
        }
    }

}

void PvLine::merge(PvLine pv) {

    unsigned i;
    for (i = 0; i < pv.size; i++) {
        line[size + i] = pv.line[i];
    }
    size += i;

}

void PvLine::append(const Move move) {

    line[size] = move;
    ++size;

}

bool PvLine::compare(const PvLine& pv) const {

    if (pv.size > size) return false;

    for (unsigned int pcount = 0; pcount < pv.size; pcount++) {
        if (line[pcount] != pv.line[pcount]) return false;
    }
    return true;

}

void PvLine::clear() {

    std::fill(line, line + MAX_DEPTH, MOVE_NONE);

}

void clearHistory(SearchInfo* info) {

    for (unsigned int pt = WHITE_PAWN; pt <= BLACK_KING; pt++) {
        for (unsigned int sq = 0; sq < 64; sq++) {
            info->history[pt][sq] = 0;
            info->countermove[pt][sq] = MOVE_NONE;
        }
    }

}

void clearKillers(SearchInfo* info) {

    for (int i = 0; i < MAX_DEPTH; i++) {
        info->killers[i][0] = MOVE_NONE;
        info->killers[i][1] = MOVE_NONE;
    }

}

static void checkUp(SearchInfo* info) {

    if (info->limitTime) {

        if (is_time_exceeded(info)) {
            info->stopped = true;
        }

    }

}

uint64_t Board::getLeastValuablePiece(uint64_t attackers, const Color color, Piecetype& pt) const {

    for (pt = Pawn(color); pt <= King(color); pt += 2) {
        uint64_t subset = attackers & bitboards[pt];
        if (subset) {
            return subset & -subset;
        }
    }

    return 0;

}

int Board::see(const Move move, Color color) const {

    int value[32];
    unsigned int d = 0;

    unsigned int fromsq = from_sq(move);
    unsigned int tosq   = to_sq(move);

    Piecetype attacker = piecetypes[fromsq];

    uint64_t mayXray = pieces(PAWN, WHITE) | pieces(PAWN, BLACK) | pieces(BISHOP, WHITE) | pieces(BISHOP, BLACK) | pieces(ROOK, WHITE) | pieces(ROOK, BLACK) | pieces(QUEEN, WHITE) | pieces(QUEEN, BLACK);
    uint64_t fromset = SQUARES[fromsq];

    uint64_t occupied  = pieces(ALLPIECES);
    uint64_t attackers = all_attackers(tosq, occupied);

    if (is_ep(move)) {
        value[d] = SeeMaterial[PAWN];
    } else {
        value[d] = SeeMaterial[piecetypes[tosq]];
    }

    do {

        color = !color;
        d++;
        value[d] = SeeMaterial[attacker] - value[d - 1];

        if (-value[d - 1] < 0 && value[d] < 0) {
            return value[d];
        }

        attackers ^= fromset;
        occupied  ^= fromset;

        if (fromset & mayXray) {
            attackers |= all_slider_attackers(tosq, occupied) & occupied;
        }

        fromset = getLeastValuablePiece(attackers, color, attacker);

    } while (fromset);

    while (--d) {
        value[d - 1] = -((-value[d-1] > value[d]) ? -value[d-1] : value[d]);
    }

    return value[0];

}

static void update_quiet_stats(const Board& board, SearchInfo *info, const int plies, const int depth, const MoveList& quiets, const Move bestMove) {

    int bonus = std::min(400, depth * depth);

    if (bestMove != info->killers[plies][0]) {
        info->killers[plies][1] = info->killers[plies][0];
        info->killers[plies][0] = bestMove;
    }

    if (info->currentmove[plies-1] != MOVE_NONE) {
        const unsigned prevsq = to_sq(info->currentmove[plies-1]);
        info->countermove[board.piecetype(prevsq)][prevsq] = bestMove;
    }

    for (unsigned i = 0; i < quiets.size; i++) {
        Move move = quiets.moves[i];
        unsigned fromSq = from_sq(move);
        unsigned toSq = to_sq(move);

        int delta = (move == bestMove) ? bonus : -bonus;
        int value = info->history[board.piecetype(fromSq)][toSq];
        info->history[board.piecetype(fromSq)][toSq] += 32 * delta - value * std::abs(delta) / 512;
    }

}

static int qsearch(int alpha, int beta, int depth, int plies, Board& board, SearchInfo *info) {

    assert(alpha >= -VALUE_INFINITE && beta <= VALUE_INFINITE && alpha < beta);

    info->nodes++;
    info->selectiveDepth = std::max(info->selectiveDepth, plies);

    if ((info->nodes & 1023) == 1023) {
        checkUp(info);
    }

    if (info->stopped || board.checkDraw()) {
        return VALUE_DRAW;
    }

    const bool pvNode = (beta - alpha != 1);
    Move ttMove = MOVE_NONE;

    int bestValue, value, eval;
    bool ttHit;

    const int oldAlpha = alpha;
    const bool inCheck = board.checkers();
    const int ttDepth = (inCheck || depth >= 0) ? 0 : -1;

    info->currentmove[plies] = MOVE_NONE;

    TTEntry * entry = tTable.probe(board.hashkey(), ttHit);

    if (!pvNode && ttHit && entry->depth >= ttDepth) {

        const int ttValue = value_from_tt(entry->value, plies);
        ttMove = board.is_valid(entry->bestMove) ? entry->bestMove : MOVE_NONE;

        if (   ttValue != VALUE_NONE
            && ((entry->flag == BOUND_EXACT)
             || (entry->flag == BOUND_UPPER && ttValue <= alpha)
             || (entry->flag == BOUND_LOWER && ttValue >= beta)))
        {
            return ttValue;
        }

    }

    bestValue = eval = (ttHit && entry->eval != VALUE_NONE ? entry->eval : evaluate(board));

    if (bestValue >= beta) {
        return bestValue;
    }

    if (bestValue > alpha) {
        alpha = bestValue;
    }

    int moveCount = 0;
    Move bestMove = MOVE_NONE;
    Move move = MOVE_NONE;

    MovePicker picker(board, info, plies, ttMove);
    picker.phase = TTMoveQS;

    while ( (move = picker.pick() ) != MOVE_NONE )  {

        if (eval + DeltaMaterial[board.piecetype(to_sq(move))] < alpha - DELTA_MARGIN) {
            continue;
        }

        moveCount++;

        if (!board.is_legal(move)) {
            moveCount--;
            continue;
        }

        board.do_move(move);

        info->currentmove[plies] = move;

        value = -qsearch(-beta, -alpha, depth - 1, plies + 1, board, info);

        board.undo_move();

        if (value > bestValue) {
            bestValue = value;
            if (value > alpha) {
                bestMove = move;
                if (pvNode && value < beta) {
                    alpha = value;
                } else {
                    break;
                }
            }
        }

    }

    if (inCheck && moveCount == 0) {
        return -VALUE_MATE + plies;
    }

    tTable.store(board.hashkey(), ttDepth, value_to_tt(bestValue, plies), eval, bestMove, bestValue >= beta ? BOUND_LOWER : pvNode && bestValue > oldAlpha ? BOUND_EXACT : BOUND_UPPER);
    return bestValue;

}

static int search(int alpha, int beta, int depth, int plies, bool cutNode, Board& board, SearchInfo *info, PvLine& pv, bool pruning, Move excluded = MOVE_NONE) {

    if ((info->nodes & 1023) == 1023) {
        checkUp(info);
    }

    if (depth <= 0) {

        return qsearch(alpha, beta, 0, plies, board, info);

    } else {

        assert(alpha >= -VALUE_INFINITE && beta <= VALUE_INFINITE && alpha < beta);
        assert(depth > 0 && depth <= MAX_DEPTH);

        info->nodes++;
        info->selectiveDepth = std::max(info->selectiveDepth, plies);

        if (info->stopped || board.checkDraw()) {
            return VALUE_DRAW;
        }

        const bool rootNode = (plies == 0);
        const bool pvNode   = (beta - alpha != 1);

        PvLine newpv;
        MoveList quietMoves;
        bool ttHit = false;
        bool improving = false;
        int value, bestValue, eval;
        int ttValue = VALUE_NONE;
        value = bestValue = -VALUE_INFINITE;
        eval = info->eval[plies] = VALUE_NONE;

        info->currentmove[plies] = MOVE_NONE;
        info->killers[plies + 1][0] = info->killers[plies + 1][1] = MOVE_NONE;

        TTEntry * entry;
        Move ttMove = MOVE_NONE;
        Move bestMove = MOVE_NONE;

        if (excluded == MOVE_NONE) {

            entry = tTable.probe(board.hashkey(), ttHit);

            if (ttHit) {

                ttMove = board.is_valid(entry->bestMove) ? entry->bestMove : MOVE_NONE;
                ttValue = value_from_tt(entry->value, plies);

                if (!pvNode && entry->depth >= depth) {
                    if ((entry->flag == BOUND_EXACT)
                     || (entry->flag == BOUND_UPPER && ttValue <= alpha)
                     || (entry->flag == BOUND_LOWER && ttValue >= beta)) {
                        pv.append(ttMove);
                        return ttValue;
                    }
                }

            }

        }

        const bool inCheck = board.checkers();

        // Static Eval
        if (!inCheck) {

            if (ttHit && entry->eval != VALUE_NONE) {
                eval = info->eval[plies] = entry->eval;
            } else {
                eval = info->eval[plies] = evaluate(board);
                tTable.store(board.hashkey(), DEPTH_NONE, VALUE_NONE, eval, MOVE_NONE, BOUND_NONE);
            }

        }

        if (pruning) {

            // Razoring
            if (   !rootNode
                && depth < 2
                && eval <= alpha - RazorMargin[depth])
            {
                return qsearch(alpha, beta, 0, plies, board, info);
            }

            // Null move pruning
            if (!pvNode && depth >= 2 && !inCheck && board.minors_or_majors(board.turn()) && eval >= beta) {

                board.do_nullmove();
                value = -search(-beta, -beta + 1, depth - (2 + (32 * depth + std::min(eval - beta, 512)) / 128), plies + 1, !cutNode, board, info, newpv, false);
                board.undo_nullmove();

                if (value >= beta && std::abs(value) < (VALUE_MATE - MAX_DEPTH)) {
                    return beta;
                }

            }

        }

        int moveCount = 0;
        improving = plies >= 2 && (eval >= info->eval[plies - 2] || info->eval[plies - 2] == VALUE_NONE);

        // Internal Iterative Deepening
        if (pvNode && !inCheck && ttMove == MOVE_NONE && depth >= 6) {

            value = search(alpha, beta, depth - 2, plies + 1, cutNode, board, info, newpv, pruning);

            entry = tTable.probe(board.hashkey(), ttHit);

            if (ttHit) {
                ttMove = board.is_valid(entry->bestMove) ? entry->bestMove : MOVE_NONE;
            }
        }

        MovePicker picker(board, info, plies, ttMove);

        Move move;

        while ( (move = picker.pick() ) != MOVE_NONE) {

            if (move == excluded)
                continue;

            bool capture = board.is_capture(move);
            bool givescheck = board.gives_check(move);
            bool promotion = is_promotion(move);

            if (!capture && !promotion) {
                quietMoves.append(move);
            }

            if (   !capture
                && !givescheck
                && !promotion)
            {

                // Futility Pruning
                if (!pvNode && !inCheck && moveCount > 0 && depth <= 5 && eval + FutilityMargin[depth] <= alpha) {
                    continue;
                }

            }

            moveCount++;

            int reductions = 0;
            int extensions = 0;

            // Singular extensions
            if (   depth >= 8
                && move == ttMove
                && excluded == MOVE_NONE
                && !rootNode
                && ttValue != VALUE_NONE
                && entry->flag == BOUND_LOWER
                && entry->depth >= depth - 3
                && board.is_legal(move))
            {
                int rbeta = std::max(ttValue - 2 * depth, -VALUE_MATE);
                value = search(rbeta - 1, rbeta, depth / 2, plies + 1, cutNode, board, info, newpv, false, move);
                if (value < rbeta)
                    extensions = 1;
            } else {
                // Check extension
                if (inCheck && board.see(move, board.turn()) >= 0) {
                    extensions = 1;
                }
            }

            if (!board.is_legal(move)) {
                moveCount--;
                continue;
            }

            board.do_move(move);

            info->currentmove[plies] = move;

            newpv.size = 0;

            // Late Move Reduction
            if (moveCount > 1) {
                if (   !capture
                    && !givescheck
                    && !promotion
                    && depth >= 3)
                {

                    reductions = LMRTable[depth][moveCount];

                    reductions -= pvNode;

                    reductions += cutNode;

                    reductions -= (move == info->killers[plies][0] || move == info->killers[plies][1] || move == picker.counterMove);

                    reductions -= inCheck;

                    reductions -= std::min(1, info->history[board.piecetype(to_sq(move))][to_sq(move)] / 512);

                    reductions = std::max(0, std::min(reductions, depth - 2));

                }

                value = -search(-alpha - 1, -alpha, depth - 1 + extensions - reductions, plies + 1, true, board, info, newpv, pruning);
                if (value > alpha && reductions) {
                    value = -search(-alpha - 1, -alpha, depth - 1 + extensions, plies + 1, !cutNode, board, info, newpv, pruning);
                }
                if (value > alpha && value < beta) {
                    value = -search(-beta, -alpha, depth - 1 + extensions, plies + 1, false, board, info, newpv, pruning);
                }

            } else {

                value = -search(-beta, -alpha, depth - 1 + extensions, plies + 1, (pvNode ? false : !cutNode), board, info, newpv, pruning);

            }

            board.undo_move();

            if (info->stopped)
                return VALUE_DRAW;

            if (value > bestValue) {
                bestValue = value;
                if (value > alpha) {
                    alpha = value;
                    bestMove = move;
                    pv.size = 0;
                    pv.append(move);
                    pv.merge(newpv);
                    if (value >= beta) {
                        break;
                    }

                }

            }

        }

        if (moveCount == 0)
            return excluded != MOVE_NONE ? alpha : inCheck ? -VALUE_MATE + plies : VALUE_DRAW;

        if (bestValue >= beta && !is_promotion(bestMove) && !board.is_capture(bestMove)) {
            update_quiet_stats(board, info, plies, depth, quietMoves, bestMove);
        }

        if (excluded == MOVE_NONE)
            tTable.store(board.hashkey(), depth, value_to_tt(bestValue, plies), eval, bestMove, bestValue >= beta ? BOUND_LOWER : pvNode && bestMove != MOVE_NONE ? BOUND_EXACT : BOUND_UPPER);
        return bestValue;

    }

}

// Find a best move for the given board in time/depth
const SearchStats go(Board& board, const SearchLimits& limits) {

    SearchStats stats;
    SearchInfo info;

    PvLine pv;
    pv.clear();
    Move bestMove = MOVE_NONE;
    int value = 0;

    info.start = Clock::now();
    init_time_management(limits, &info);

    clearHistory(&info);
    clearKillers(&info);

    for (unsigned depth = 1; depth < limits.depth; depth++) {

        pv.size = 0;

        board.reset_plies();

        info.depth = depth;
        info.selectiveDepth = 0;

        if (depth > 5) {

            int window = 25 - std::min(depth / 3, 10u) + std::abs(value) / 25;
            int alpha = value - window;
            int beta = value + window;

            while (true) {

                value = search(alpha, beta, depth, 0, false, board, &info, pv, true);

                if (value <= alpha) {
                    beta = (alpha + beta) / 2;
                    alpha = value - window;
                } else if (value >= beta) {
                    beta = value + window;
                } else {
                    break;
                }

                window += window / 4;

                pv.clear();

            }

        } else {
            value = search(-VALUE_INFINITE, VALUE_INFINITE, depth, 0, false, board, &info, pv, true);
        }

        info.value[depth] = value;
        long long duration = get_time_elapsed(info.start);

        if (!info.stopped) {
            // Drawn/mate positions return an empty pv
            if (pv.size > 0) {

                bestMove = pv.line[0];
                info.bestmove[depth] = bestMove;
                send_info(&info, pv, duration);

            }  else {
                send_info(&info, pv, duration);
                break;
            }

            update_time_managemement(&info);

            if (info.limitTime) {
                if (duration >= limits.time || should_stop(info)) {
                    break;
                }
            }
        } else {
            break;
        }

    }

    send_bestmove(bestMove);

    stats.totalNodes = info.nodes;

    return stats;

}
