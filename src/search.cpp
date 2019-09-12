/*
  Delocto Chess Engine
  Copyright (c) 2018 Moritz Terink

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

static int LMRTable[MAX_DEPTH][MAX_MOVES];

void init_search() {

    for (int d = 1; d < MAX_DEPTH; d++) {
        for (int m = 1; m < MAX_MOVES; m++) {
            LMRTable[d][m] = 1 + log(d) * log(m) / 2;
        }
    }

}

void PvLine::merge(PvLine pv) {

    int i;
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

void clearHistory(SearchInfo * info) {

    for (unsigned int pt = WHITE_PAWN; pt <= BLACK_KING; pt++) {
        for (unsigned int sq = 0; sq < 64; sq++) {
            info->history[pt][sq] = 0;
            info->countermove[pt][sq] = MOVE_NONE;
        }
    }

}

void clearKillers(SearchInfo * info) {

    for (int i = 0; i < MAX_DEPTH; i++) {
        info->killers[i][0] = MOVE_NONE;
        info->killers[i][1] = MOVE_NONE;
    }

}

static void checkUp(SearchInfo * info) {

    if (info->limitTime) {

        clock_t now = std::clock();

        if (((now - info->start) / (CLOCKS_PER_SEC / 1000)) > info->timeLeft) {
            info->stopped = true;
        }

    }

}

uint64_t Board::getLeastValuablePiece(uint64_t attackers, const Side side, PieceType& pt) const {

    for (pt = Pawn(side); pt <= King(side); pt += 2) {
        uint64_t subset = attackers & bitboards[pt];
        if (subset) {
            return subset & -subset;
        }
    }

    return 0;

}

int Board::see(const Move move, Side side) const {

    int value[32];
    unsigned int d = 0;

    unsigned int fromsq = from_sq(move);
    unsigned int tosq   = to_sq(move);

    PieceType attacker = piecetypes[fromsq];

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

        side = !side;
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

        fromset = getLeastValuablePiece(attackers, side, attacker);

    } while (fromset);

    while (--d) {
        value[d - 1] = -((-value[d-1] > value[d]) ? -value[d-1] : value[d]);
    }

    return value[0];

}

void MovePicker::scoreCaptures() {

    for (unsigned int index = 0; index < caps.size; index++) {
        caps.scores[index] = board.mvvlva(caps.moves[index]);
    }

}

void MovePicker::scoreQuiets() {

}

void MovePicker::scoreQsCaptures() {

    for (unsigned int index = 0; index < qscaps.size; index++) {
        qscaps.scores[index] = board.mvvlva(qscaps.moves[index]);
    }

}

Move MovePicker::pick() {

    switch (phase) {

        case HashMove:

            {
                ++phase;

                // Hashmove validity tested before
                if (ttMove != MOVE_NONE) {
                    return ttMove;
                }

            }

        case GenCaps:

            {

                ++phase;

                caps = gen_caps(board, board.turn());
                scoreCaptures();

            }

        case GoodCaps:

            {

                while (caps.index < caps.size) {

                    Move best = caps.pick();

                    assert(best == MOVE_NONE);

                    if (caps.scores[caps.index] < 0) {
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

            if (countermove != ttMove && countermove != MOVE_NONE && countermove != killers[0] && countermove != killers[1] && !board.is_capture(countermove) && board.is_valid(countermove)) {
                return countermove;
            }

        case GenQuiets:

            {
                ++phase;

                quiets = gen_quiets(board, board.turn());

                for (unsigned int index = 0; index < quiets.size; index++) {
                    quiets.scores[index] = info->history[board.piecetype(from_sq(quiets.moves[index]))][to_sq(quiets.moves[index])];
                }

            }

        case Quiets:

            {

                while (quiets.index < quiets.size) {

                    Move best = quiets.pick();
                    assert(best == MOVE_NONE);
                    ++quiets.index;

                    if (best != ttMove && best != killers[0] && best != killers[1] && best != countermove) {

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
                scoreQsCaptures();

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

static void update_quiet_stats(const Board& board, SearchInfo *info, const int plies, const int depth, const Move move) {

    if (move != info->killers[plies][0]) {
        info->killers[plies][1] = info->killers[plies][0];
        info->killers[plies][0] = move;
    }

    if (info->currentmove[plies-1] != MOVE_NONE) {
        int prevsq = to_sq(info->currentmove[plies-1]);
        info->countermove[board.piecetype(prevsq)][prevsq] = move;
    }

    info->history[board.piecetype(from_sq(move))][to_sq(move)] += depth * depth;

}

static int quiescence(int alpha, int beta, int depth, int plies, Board& board, SearchInfo *info) {

    info->nodes++;

    if (info->stopped) return VALUE_NONE;

    if ((info->nodes & 1023) == 1023) {
        checkUp(info);
    }

    if (board.checkDraw()) {
        return VALUE_DRAW;
    }

    int bestValue, value, eval;
    int ttValue = VALUE_NONE;
    bool ttHit;

    TTEntry * entry = tTable.probe(board.hashkey(), ttHit);

    if (ttHit) {

        ttValue = value_from_tt(entry->value, plies);

        if (   (entry->flag == BOUND_EXACT)
            || (entry->flag == BOUND_UPPER && ttValue <= alpha)
            || (entry->flag == BOUND_LOWER && ttValue >= beta))
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

    int movecount = 0;
    Move bestMove = MOVE_NONE;
    Move move = MOVE_NONE;
    plies++;

    MovePicker picker(board, info, plies);
    picker.phase = GenCapsQS;
    int hashtype = BOUND_UPPER;

    while ( (move = picker.pick() ) != MOVE_NONE )  {

        if (eval + DeltaMaterial[board.piecetype(to_sq(move))] < alpha - DELTA_MARGIN)
        {
            continue;
        }

        movecount++;

        if (!board.is_legal(move)) {
            movecount--;
            continue;
        }

        board.do_move(move);

        info->currentmove[plies] = move;

        value = -quiescence(-beta, -alpha, depth - 1, plies, board, info);

        board.undo_move();

        if (value > bestValue) {
            bestValue = value;
            if (value >= beta) {
                return beta;
            }

            if (value > alpha) {
                alpha = value;
                bestMove = move;
                hashtype = BOUND_EXACT;
            }
        }

    }
    //tTable.store(board.hashkey(), depth, bestValue, bestMove, hashtype);
    return bestValue;

}

static int alphabeta(int alpha, int beta, int depth, int plies, bool cutNode, Board& board, SearchInfo *info, PvLine& pv, bool pruning, Move excluded = MOVE_NONE) {

    if (info->stopped) return VALUE_NONE;

    if ((info->nodes & 1023) == 1023) {
        checkUp(info);
    }

    if (depth <= 0) {

        return quiescence(alpha, beta, 0, plies, board, info);

    } else {

        info->nodes++;

        if (board.checkDraw()) {
            return VALUE_DRAW;
        }

        const bool rootNode = (plies == 0);
        const bool pvNode   = (beta - alpha != 1);

        plies++;

        PvLine newpv;
        bool ttHit = false;
        bool improving = false;
        int value, bestValue, eval;
        int ttValue = VALUE_NONE;
        value = bestValue = -VALUE_INFINITE;
        eval = info->eval[plies] = VALUE_NONE;

        info->currentmove[plies] = MOVE_NONE;
        info->killers[plies + 1][0] = info->killers[plies + 1][1] = MOVE_NONE;
        int prevsq = to_sq(info->currentmove[plies - 1]);

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

        const bool incheck = board.checkers();

        // Static Eval
        if (!incheck) {

            if (ttHit && entry->eval != VALUE_NONE) {
                eval = info->eval[plies] = entry->eval;
            } else {
                eval = info->eval[plies] = evaluate(board);
                tTable.store(board.hashkey(), DEPTH_NONE, VALUE_NONE, eval, MOVE_NONE, BOUND_NONE);
            }

        }

        if (pruning) {

            // Razoring
            if (   !pvNode
                && !incheck
                && depth <= 4
                && eval <= alpha - RazorMargin[depth])
            {
                int ralpha = alpha - RazorMargin[depth];
                int value = quiescence(ralpha, ralpha + 1, 0, plies, board, info);
                if (value <= ralpha)
                    return value;
            }

            // Null move pruning
            if (!pvNode && depth >= 2 && !incheck && board.minors_or_majors(board.turn()) && eval >= beta) {

                board.do_nullmove();
                value = -alphabeta(-beta, -beta + 1, depth - (2 + (32 * depth + std::min(eval - beta, 512)) / 128), plies, !cutNode, board, info, newpv, false);
                board.undo_nullmove();

                if (value >= beta && std::abs(value) < (VALUE_MATE - MAX_DEPTH)) {
                    return beta;
                }

            }

        }

        int movecount = 0;
        improving = plies >= 2 && (eval >= info->eval[plies - 2] || info->eval[plies - 2] == VALUE_NONE);

        // Internal Iterative Deepening
        if (pvNode && !incheck && ttMove == MOVE_NONE && depth >= 6) {

            value = alphabeta(alpha, beta, depth - 2, plies, cutNode, board, info, newpv, pruning);

            entry = tTable.probe(board.hashkey(), ttHit);

            if (ttHit) {
                ttMove = board.is_valid(entry->bestMove) ? entry->bestMove : MOVE_NONE;
            }
        }

        MovePicker picker(board, info, plies, info->countermove[board.piecetype(prevsq)][prevsq]);

        if (ttMove != MOVE_NONE) {
            picker.ttMove = ttMove;
        } else {
            picker.phase++;
        }

        Move move;

        while ( (move = picker.pick() ) != MOVE_NONE) {

            if (move == excluded)
                continue;

            bool capture = board.is_capture(move);
            bool givescheck = board.gives_check(move);
            bool promotion = is_promotion(move);


            if (   !capture
                && !givescheck
                && !promotion)
            {

                // Futility Pruning
                if (!pvNode && !incheck && movecount > 0 && depth <= 5 && eval + FutilityMargin[depth] <= alpha) {
                    continue;
                }

            }

            movecount++;

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
                value = alphabeta(rbeta - 1, rbeta, depth / 2, plies, cutNode, board, info, newpv, false, move);
                if (value < rbeta)
                    extensions = 1;
            } else {
                // Check extension
                if (incheck && board.see(move, board.turn()) >= 0) {
                    extensions = 1;
                }
            }

            if (!board.is_legal(move)) {
                movecount--;
                continue;
            }

            board.do_move(move);

            info->currentmove[plies] = move;

            newpv.size = 0;

            // Late Move Reduction
            if (movecount > 1) {
                if (   !capture
                    && !givescheck
                    && !promotion
                    && depth >= 3)
                {

                    reductions = LMRTable[depth][movecount];

                    reductions -= pvNode;

                    reductions += cutNode;

                    reductions -= (move == picker.killers[0] || move == picker.killers[1] || move == picker.countermove);

                    reductions -= incheck;

                    reductions -= std::min(1, info->history[board.piecetype(from_sq(move))][to_sq(move)] / 512);

                    reductions = std::max(0, std::min(reductions, depth - 2));

                }

                value = -alphabeta(-alpha - 1, -alpha, depth - 1 + extensions - reductions, plies, true, board, info, newpv, pruning);
                if (value > alpha && reductions) {
                    value = -alphabeta(-alpha - 1, -alpha, depth - 1 + extensions, plies, !cutNode, board, info, newpv, pruning);
                }
                if (value > alpha && value < beta) {
                    value = -alphabeta(-beta, -alpha, depth - 1 + extensions, plies, false, board, info, newpv, pruning);
                }

            } else {

                value = -alphabeta(-beta, -alpha, depth - 1 + extensions, plies, (pvNode ? false : !cutNode), board, info, newpv, pruning);

            }

            board.undo_move();

            if (info->stopped)
                return VALUE_NONE;

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

        if (movecount == 0)
            return excluded != MOVE_NONE ? alpha : incheck ? -VALUE_MATE + plies : VALUE_DRAW;

        if (bestValue >= beta && !is_promotion(bestMove) && !board.is_capture(bestMove)) {
            update_quiet_stats(board, info, plies, depth, bestMove);
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
    Move bestMove = MOVE_NONE;
    clock_t start, end, iterend;
    uint64_t duration, iterduration;
    std::string pvstring;
    int value = 0;
    int depth;

    if (limits.moveTime != -1) {
        info.timeLeft = limits.moveTime;
    } else if (board.turn() == WHITE && (limits.whiteTime != -1 || limits.whiteIncrement != -1)) {
        info.timeLeft = (limits.whiteTime / 20) + limits.whiteIncrement;
    } else if (board.turn() == BLACK && (limits.blackTime != -1 || limits.blackIncrement != -1)) {
        info.timeLeft = (limits.blackTime / 20) + limits.blackIncrement;
    } else {
        info.limitTime = false;
    }

    start = std::clock();
    info.start = start;

    PvLine pv;
    pv.clear();

    info.lastPv.clear();
    clearHistory(&info);
    clearKillers(&info);

    for (int depth = 1; depth < limits.depth; depth++) {

        pv.size = 0;

        board.reset_plies();

        info.curdepth = depth;

        clock_t iterstart = std::clock();

        if (depth > 5) {

            int window = 25 - std::min(depth / 3, 10) + std::abs(value) / 25;
            int alpha = value - window;
            int beta = value + window;

            while (true) {

                value = alphabeta(alpha, beta, depth, 0, false, board, &info, pv, true);

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

            value = alphabeta(-VALUE_INFINITE, VALUE_INFINITE, depth, 0, false, board, &info, pv, true);

        }

        if (!info.stopped) {

            iterend = std::clock();

            if (info.limitTime) {
                iterduration = (iterend - iterstart) / (CLOCKS_PER_SEC / 1000);
                info.timeLeft -= iterduration;
            }

            // Drawn/mate positions return an empty pv
            if (pv.size > 0) {

                info.lastPv = pv;

                end = std::clock();

                bestMove = pv.line[0];

                duration = (end - start) / (CLOCKS_PER_SEC / 1000);

                pvstring.clear();

                for (int pcount = 0; pcount < pv.size; pcount++) {
                    pvstring += std::string() + move_to_string(pv.line[pcount]) + ' ';
                }

                std::cout << "info depth " << depth;

                if (std::abs(value) >= VALUE_MATE - MAX_DEPTH) {
                    std::cout << " score mate " << ((value > 0 ? VALUE_MATE - value + 1 : -VALUE_MATE - value) / 2);
                } else {
                    std::cout << " score cp " << value;
                }

                std::cout << " nodes " << info.nodes << " time " << duration << " nps " << ((duration != 0) ? ((info.nodes * 1000) / duration) : info.nodes * 1000) << " pv " << pvstring << std::endl;

                if (info.limitTime && (info.timeLeft <= 0 || info.timeLeft <= iterduration * (1 + depth / 50)))
                    break;

            }  else {

                std::cout << "info string No legal moves available" << std::endl;
                break;

            }

        } else {

            break;

        }

    }

    stats.totalNodes = info.nodes;

    if (bestMove != MOVE_NONE) {
        std::cout << "bestmove " << move_to_string(bestMove) << std::endl;
    }

    return stats;

}
