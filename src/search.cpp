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

// #define INFO_OUTPUT

void PvLine::merge(PvLine pv) {

    unsigned int pcount;
    for (pcount = 0; pcount < pv.size; pcount++) {
        line[size + pcount] = pv.line[pcount];
    }
    size += pcount;

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

    int pcount;
    for (pcount = 0; pcount < MAXDEPTH; pcount++) {
        line[pcount] = NOMOVE;
    }

    size = 0;

}

void clearHistory(SearchInfo * info) {

    for (unsigned int pt = WHITE_PAWN; pt <= BLACK_KING; pt++) {
        for (unsigned int sq = 0; sq < 64; sq++) {
            info->history[pt][sq] = 0;
            info->countermove[pt][sq] = NOMOVE;
        }
    }

}

void clearKillers(SearchInfo * info) {

    for (unsigned int kcount = 0; kcount < MAXDEPTH; kcount++) {
        info->killers[kcount][0] = NOMOVE;
        info->killers[kcount][1] = NOMOVE;
    }

}

static void checkUp(SearchInfo * info) {

    if (info->limit == TIME_LIMIT || info->limit == MOVETIME_LIMIT) {

        clock_t now = std::clock();

        if (((now - info->start) / (CLOCKS_PER_SEC / 1000)) > info->timeLeft) {
            info->stopped = true;
        }

    }

}

static bool checkUCI() {

    std::string input;

    std::cin >> input;

    if (input.compare("stop") == 0) {
        return true;
    } else if (input.compare("isready") == 0) {
        std::cout << "readyok" << std::endl;
    }

    return false;

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
                if (hashmove != NOMOVE) {
                    return hashmove;
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

                    assert(best == NOMOVE);

                    if (caps.scores[caps.index] < 0) {
                        break;
                    }

                    caps.index++;

                    if (best != hashmove) {
                        return best;
                    }

                }

                ++phase;

            }

        case FirstKiller:

            ++phase;

            // Note: Checking wether killer is a capture, since it could have already been picked during GoodCaps
            if (killers[0] != hashmove && killers[0] != NOMOVE && !board.is_capture(killers[0]) && board.is_valid(killers[0])) {
                return killers[0];
            }

        case SecondKiller:

            ++phase;

            if (killers[1] != hashmove && killers[1] != NOMOVE && !board.is_capture(killers[1]) && board.is_valid(killers[1])) {
                return killers[1];
            }

        case CounterMove:

            ++phase;

            if (countermove != hashmove && countermove != NOMOVE && countermove != killers[0] && countermove != killers[1] && !board.is_capture(countermove) && board.is_valid(countermove)) {
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
                    assert(best == NOMOVE);
                    ++quiets.index;

                    if (best != hashmove && best != killers[0] && best != killers[1] && best != countermove) {

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
                    if (best != hashmove && best != killers[0] && best != killers[1]) {

                        return best;

                    }

                }

                return NOMOVE;

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
                    return NOMOVE;

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

static int quiescence(int alpha, int beta, int depth, int plies, Board& board, SearchInfo *info) {

    info->nodes++;

    if (info->stopped) return UNKNOWNVALUE;

    if ((info->nodes & 1023) == 1023) {
        checkUp(info);
    }

    if (board.checkDraw()) {
        return DRAWVALUE;
    }

    int bestScore, score, eval;
    bestScore = eval = evaluate(board);

    if (eval >= beta) {
        return beta;
    }

    if (eval > alpha) {
        alpha = score;
    }

    int movenum = 0;
    Move bestMove = NOMOVE;
    Move move = NOMOVE;
    plies++;

    MovePicker picker(board, info, plies);
    picker.phase = GenCapsQS;
    int hashtype = ALPHAHASH;

    while ( (move = picker.pick() ) != NOMOVE )  {

        if (eval + DeltaMaterial[board.piecetype(to_sq(move))] < alpha - DELTA_MARGIN) {
            continue;
        }

        movenum++;

        if (!board.is_legal(move)) {
            movenum--;
            continue;
        }

        board.do_move(move);

        info->currentmove[plies] = move;

        score = -quiescence(-beta, -alpha, depth - 1, plies, board, info);

        board.undo_move();

        if (score > bestScore) {
            bestScore = score;
            if (score >= beta) {
            #ifdef INFO_OUTPUT
                if (movenum == 1) {
                    info->fhf++;
                }
                info->fh++;
            #endif
                tTable.store(board.hashkey(), depth, beta, move, BETAHASH);
                return beta;
            }

            if (score > alpha) {
                alpha = score;
                bestMove = move;
                hashtype = EXACTHASH;
            }
        }

    }
    tTable.store(board.hashkey(), depth, bestScore, bestMove, hashtype);
    return bestScore;

}

static int alphabeta(int alpha, int beta, int depth, int plies, bool cutnode, Board& board, SearchInfo *info, PvLine& pv, bool pruning, Move excluded = NOMOVE) {

    int score = -INFINITE;

    if (info->stopped) return UNKNOWNVALUE;

    if ((info->nodes & 1023) == 1023) {
        checkUp(info);
    }

    if (depth <= 0) {

        score = quiescence(alpha, beta, 0, plies, board, info);
        return score;

    } else {

        info->nodes++;

        if (board.checkDraw()) {
            return DRAWVALUE;
        }

        PvLine newpv;
        int eval;

        const bool rootnode = (plies == 0);
        const bool pvnode   = (beta - alpha != 1);

        plies++;
        info->currentmove[plies] = NOMOVE;

        TTEntry * entry = NULL;

        Move hashmove = NOMOVE;

        if (excluded == NOMOVE) {

            entry = tTable.probe(board.hashkey());

            if (entry != NULL) {

                hashmove = board.is_valid(entry->bestmove) ? entry->bestmove : NOMOVE;

                #ifdef INFO_OUTPUT
                    info->hashTableHits++;
                #endif

                if (!pvnode && entry->depth >= depth) {
                    if ((entry->flag == EXACTHASH)
                     || (entry->flag == ALPHAHASH && entry->score <= alpha)
                     || (entry->flag == BETAHASH && entry->score >= beta)) {
                        pv.append(hashmove);
                        return entry->score;
                    }
                }

            }

        }

        const bool checked = board.checkers();

        // Static Eval
        if (!checked) {
            eval = evaluate(board);
        }

        if (pruning) {

            // Razoring
            if (   !pvnode
                && !checked
                && depth <= 4
                && eval <= alpha - RazorMargin[depth])
            {
                int ralpha = alpha - RazorMargin[depth];
                int score = quiescence(ralpha, ralpha + 1, 0, plies, board, info);
                if (score <= ralpha)
                    return score;
            }

            // Null move pruning
            if (!pvnode && depth >= 2 && !checked && board.minors_or_majors(board.turn()) && eval >= beta) {

                board.do_nullmove();
                score = -alphabeta(-beta, -beta + 1, depth - (2 + (32 * depth + std::min(eval - beta, 512)) / 128), plies, !cutnode, board, info, newpv, false);
                board.undo_nullmove();

                if (score >= beta && std::abs(score) < (MATEVALUE - MAXDEPTH)) {
                    return beta;
                }

            }

        }

        int hashflag = ALPHAHASH;
        int movenum = 0;
        int bestScore = -INFINITE;
        Move bestmove = NOMOVE;
        unsigned int prevsq = NOSQ;

        // Internal Iterative Deepening
        if (pvnode && !checked && hashmove == NOMOVE && depth >= 6) {

            score = alphabeta(alpha, beta, depth - 2, plies, cutnode, board, info, newpv, pruning);

            entry = tTable.probe(board.hashkey());

            if (entry != NULL) {
                hashmove = board.is_valid(entry->bestmove) ? entry->bestmove : NOMOVE;
            }
        }

        MovePicker picker(board, info, plies);

        if (hashmove != NOMOVE) {
            picker.hashmove = hashmove;
        } else {
            picker.phase++;
        }

        if (info->currentmove[plies-1] != NOMOVE) {
            prevsq = to_sq(info->currentmove[plies-1]);
            picker.countermove = info->countermove[board.piecetype(prevsq)][prevsq];
        }

        Move move;

        while ( (move = picker.pick() ) != NOMOVE) {

            if (move == excluded)
                continue;

            const bool capture = board.is_capture(move);
            const bool givescheck = board.gives_check(move);
            const bool promotion = is_promotion(move);

            const bool doFutility = !checked && !capture && !givescheck && !promotion;

            if (!pvnode && movenum > 0 && depth <= 5 && doFutility) {
                if ((eval + FutilityMargin[depth]) <= alpha) {
                    continue;
                }
            }

            movenum++;

            int reductions = 0;
            int extensions = 0;

            // Singular extensions
            if (   depth >= 8
                && move == hashmove
                && excluded == NOMOVE
                && !rootnode
                && entry->score != UNKNOWNVALUE
                && entry->flag == BETAHASH
                && entry->depth >= depth - 3
                && board.is_legal(move))
            {
                int rbeta = std::max(entry->score - 2 * depth, -MATEVALUE);
                score = alphabeta(rbeta - 1, rbeta, depth / 2, plies, cutnode, board, info, newpv, false, move);
                if (score < rbeta)
                    extensions = 1;
            } else {
                // Check extension
                if (checked && board.see(move, board.turn()) >= 0) {
                    extensions = 1;
                }
            }

            // Late Move Reduction
            if (   !capture
                && !givescheck
                && !promotion
                && depth >= 3
                && movenum > (pvnode ? 4 : 2))
            {

                reductions = (std::max(9, depth) / 6) + (movenum - 4) / 4;

                reductions += !pvnode;

                reductions -= (move == picker.killers[0] || move == picker.killers[1]);

                reductions -= checked;

                reductions -= std::min(1, info->history[board.piecetype(from_sq(move))][to_sq(move)] / 512);

                reductions = std::max(0, std::min(reductions, depth - 2));
            }

            if (!board.is_legal(move)) {
                movenum--;
                continue;
            }

            board.do_move(move);

            info->currentmove[plies] = move;

            newpv.size = 0;

            if (movenum > 1) {
                score = -alphabeta(-alpha - 1, -alpha, depth - 1 + extensions - reductions, plies, true, board, info, newpv, pruning);
                if (score > alpha && reductions) {
                    score = -alphabeta(-alpha - 1, -alpha, depth - 1 + extensions, plies, !cutnode, board, info, newpv, pruning);
                }
                if (score > alpha && score < beta) {
                    score = -alphabeta(-beta, -alpha, depth - 1 + extensions, plies, false, board, info, newpv, pruning);
                }
            } else {
                score = -alphabeta(-beta, -alpha, depth - 1 + extensions, plies, (pvnode ? false : !cutnode), board, info, newpv, pruning);
            }

            board.undo_move();

            if (info->stopped)
                return UNKNOWNVALUE;

            if (score > bestScore) {
                bestScore = score;
                if (score >= beta) {
                    if (!capture && move != info->killers[plies][0]) {
                        info->killers[plies][1] = info->killers[plies][0];
                        info->killers[plies][0] = move;
                        info->history[board.piecetype(from_sq(move))][to_sq(move)] += depth * depth;
                        if (prevsq != NOSQ)
                            info->countermove[board.piecetype(prevsq)][prevsq] = move;
                    }
                    #ifdef INFO_OUTPUT
                        if (movenum == 1) {
                            info->fhf++;
                        }
                        info->fh++;
                    #endif
                    tTable.store(board.hashkey(), depth, beta, move, BETAHASH);
                    return beta;
                }

                if (score > alpha) {
                    alpha = score;
                    bestmove = move;
                    pv.size = 0;
                    pv.append(move);
                    pv.merge(newpv);
                    hashflag = EXACTHASH;
                }

            }

        }

        if (movenum == 0) {
            if (excluded != NOMOVE) {
                return alpha;
            } else if (checked) {
                return -MATEVALUE - 1 + plies;
            } else {
                return DRAWVALUE;
            }
        }
        if (excluded == NOMOVE)
            tTable.store(board.hashkey(), depth, bestScore, bestmove, hashflag);
        return bestScore;

    }

}

// Find a bestmove for the given board in time/depth
const Move bestmove(Board& board, const unsigned int limit, const unsigned int depth, const long long timeleft, const long long increment, bool uci) {

    SearchInfo info;
    if (limit == TIME_LIMIT) {
        // Reduce increment because of possible delays between gui and engine
        info.timeLeft = (timeleft / 20) + increment;
    } else if (limit == MOVETIME_LIMIT) {
        info.timeLeft = timeleft;
    }

    info.limit = limit;

    int score = 0;

    clock_t start, end, iterend;
    long long duration, iterduration;
    std::string pvstring;

    Move bestmove = NOMOVE;

    int dcount;

    start = std::clock();
    info.start = start;

    PvLine pv;
    pv.clear();

    info.nodes = 0;
    info.lastPv.clear();
    clearHistory(&info);
    clearKillers(&info);

    for (dcount = 1; dcount < depth; dcount++) {

        #ifdef INFO_OUTPUT
            info.fhf = 0;
            info.fh = 0;
            info.hashTableHits = 0;
        #endif

        pv.size = 0;

        board.reset_plies();

        info.curdepth = dcount;

        clock_t iterstart = std::clock();

        if (dcount > 5 && std::abs(score) < MINMATE) {

            int window = 25 - std::min(dcount / 3, 10) + std::abs(score) / 25;
            int alpha = score - window;
            int beta = score + window;

            while (true) {

                score = alphabeta(alpha, beta, dcount, 0, false, board, &info, pv, true);

                if (score <= alpha) {
                    beta = (alpha + beta) / 2;
                    alpha = score - window;
                } else if (score >= beta) {
                    beta = score + window;
                } else {
                    break;
                }

                window += window / 4;

                pv.clear();

            }

        } else {

            score = alphabeta(-INFINITE, INFINITE, dcount, 0, false, board, &info, pv, true);

        }

        end:

            if (!info.stopped) {

                iterend = std::clock();

                if (info.limit == TIME_LIMIT || info.limit == MOVETIME_LIMIT) {

                    iterduration = (iterend - iterstart) / (CLOCKS_PER_SEC / 1000);
                    info.timeLeft -= iterduration;

                }

                // Drawn/mate positions return an empty pv
                if (pv.size > 0) {

                    info.lastPv = pv;

                    end = std::clock();

                    bestmove = pv.line[0];

                    if (uci) {

                        duration = (end - start) / (CLOCKS_PER_SEC / 1000);

                        pvstring.clear();

                        for (unsigned int pcount = 0; pcount < pv.size; pcount++) {

                            pvstring += std::string() + move_to_string(pv.line[pcount]) + ' ';

                        }

                        #ifdef INFO_OUTPUT
                            std::cout << "Ordering: " << std::setprecision(2) << info.fhf/info.fh << std::endl;
                            std::cout << "Hash Table Hits: " << info.hashTableHits << std::endl;
                        #endif

                        std::cout << "info depth " << dcount;

                        if (score >= (MATEVALUE - MAXDEPTH)) { std::cout << " score mate " << (((MATEVALUE - score) / 2) + 1); }
                        else if (score <= (-MATEVALUE + MAXDEPTH)) { std::cout << " score mate " << -(((MATEVALUE + score) / 2) + 1); }
                        else { std::cout << " score cp " << score; }

                        std::cout << " nodes " << info.nodes << " time " << duration << " nps " << ((duration != 0) ? ((info.nodes * 1000) / duration) : info.nodes * 1000) << " pv " << pvstring << std::endl;

                    }

                    if (limit == TIME_LIMIT && info.timeLeft < iterduration * (1 + (dcount / 50))) {
                        break;
                    } else if (limit == MOVETIME_LIMIT && info.timeLeft <= 0) {
                        break;
                    }

                }  else {
                    if (uci) {
                        std::cout << "info string No legal moves available" << std::endl;
                    } else {
                        std::cout << "error: No legal moves available" << std::endl;
                    }

                    break;
                }

            } else {

                return bestmove;

            }

    }

    return bestmove;

}
