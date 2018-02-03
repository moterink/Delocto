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
                if (hashmove != NOMOVE && board.is_legal(hashmove, board.turn())) {
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
                    
                    if (best != hashmove && board.is_legal(best, board.turn())) {
                        return best;                        
                    }
                    
                }
                
                ++phase;
                
            }
            
        case FirstKiller:
            
            ++phase;
            
            // Note: Checking wether killer is a capture, since it could have already been picked during GoodCaps
            if (killers[0] != hashmove && killers[0] != NOMOVE && !board.is_capture(killers[0]) && board.is_valid(killers[0]) && board.is_legal(killers[0], board.turn())) {
                return killers[0];
            }            
            
        case SecondKiller:
            
            ++phase;
            
            if (killers[1] != hashmove && killers[1] != NOMOVE && !board.is_capture(killers[1]) && board.is_valid(killers[1]) && board.is_legal(killers[1], board.turn())) {                
                return killers[1];
            }
			
		case CounterMove:
		
			++phase;
			
			if (countermove != hashmove && countermove != NOMOVE && countermove != killers[0] && countermove != killers[1] && !board.is_capture(countermove) && board.is_valid(countermove) && board.is_legal(countermove, board.turn())) {				
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
                    
                    if (best != hashmove && best != killers[0] && best != killers[1] && best != countermove && board.is_legal(best, board.turn())) {
                        
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
                    if (best != hashmove && best != killers[0] && best != killers[1] && board.is_legal(best, board.turn())) {
                    
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
                
                while (qscaps.index < qscaps.size) {
                
                    Move best = qscaps.pick();
                    qscaps.index++;
                    if (!board.is_legal(best, board.turn())) {
                        continue;
                    }
                    return best;
                    
                }
                
                return NOMOVE;

            }
            
        default:
			
            assert(false);
            
    }            
    
    assert(false);
    return false;
    
}

static int quiescence(int alpha, int beta, int depth, Board& board, SearchInfo *info) {
    
    info->nodes++;
    
    if (info->stopped) return UNKNOWNVALUE;
    
    if ((info->nodes & 1023) == 1023) {
        checkUp(info);
    }
    
    if (board.checkDraw()) {
        return DRAWVALUE;
    }
    
	int score, eval;
    score = eval = evaluate(board);
    
    if (score >= beta) {
        return beta;
    }
    
    if (score > alpha) {
        alpha = score;
    }
    
    int movenum = 0;
    Move move;

    MovePicker picker(board, info);
    picker.phase = GenCapsQS;
    int hashtype = ALPHAHASH;
    int bestScore = -INFINITE;
    
    while ( (move = picker.pick() ) != NOMOVE )  {
        
		if (eval + DeltaMaterial[board.piecetype(to_sq(move))] < alpha - DELTA_MARGIN) {
			continue;
		}
		
        board.do_move(move);
        
        movenum++;
        
        score = -quiescence(-beta, -alpha, depth + 1, board, info);
        
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
                tTable.store(board.hashkey(), 0, beta, move, BETAHASH);
                return beta;
            }
            
            if (score > alpha) {
                alpha = score;                
                hashtype = EXACTHASH;
            }
        }
        
    }    
    tTable.store(board.hashkey(), 0, bestScore, move, hashtype);
    return alpha;
    
}

static int alphabeta(int alpha, int beta, int depth, NodeType nodetype, Board& board, SearchInfo *info, PvLine& pv, bool nullmove) {
    
    int score = -INFINITE;
    
    if (info->stopped) return UNKNOWNVALUE;

    if ((info->nodes & 1023) == 1023) {
        checkUp(info);
    }

    if (depth <= 0) {
        
        score = quiescence(alpha, beta, 0, board, info);
        return score;
        
    } else {

        info->nodes++;
        
        if (board.checkDraw()) {
            return DRAWVALUE;
        }
        
        PvLine newpv;
        int eval;

        TTEntry * entry = tTable.probe(board.hashkey(), depth, alpha, beta);
        
        Move hashmove = NOMOVE;
        
        if (entry != NULL) {
            
            hashmove = board.is_valid(entry->bestmove) ? entry->bestmove : NOMOVE;
            
            #ifdef INFO_OUTPUT
                info->hashTableHits++;
            #endif
            
            if (nodetype != PvNode && entry->depth >= depth) {
                if ((entry->flag == EXACTHASH)
                 || (entry->flag == ALPHAHASH && entry->score <= alpha)
                 || (entry->flag == BETAHASH && entry->score >= beta)) {
                    pv.append(hashmove);
                    return entry->score;
                }
            }
            
        }
        
        const bool checked = board.checkers();
        
        // Static Eval
        if (!checked) {
            eval = evaluate(board);
        }
        
        // Null move pruning
        if (nodetype != PvNode && nullmove && depth >= 2 && !checked && board.minors_or_majors(board.turn()) && eval >= beta) {
            
            board.do_nullmove();
            score = -alphabeta(-beta, -beta + 1, depth - (2 + (32 * depth + std::min(eval - beta, 512)) / 128), CutNode, board, info, newpv, false);
            board.undo_nullmove();
            
            if (score >= beta && std::abs(score) < (MATEVALUE - MAXDEPTH)) {
                return beta;
            }
            
        }
        
        int hashflag = ALPHAHASH;
        int movenum = 0;
        int bestScore = -INFINITE;
        Move bestmove = NOMOVE;
		unsigned int pmsq = NOSQ;
        
        MovePicker picker(board, info);
        
        (hashmove != NOMOVE) ? picker.hashmove = hashmove : picker.phase++;
		if (!board.moves.empty()) {
			pmsq = to_sq(board.moves.back());
			picker.countermove = info->countermove[board.piecetype(pmsq)][pmsq];
        }

        Move move;
        
        while ( (move = picker.pick() ) != NOMOVE) {
            
			movenum++;
			
            const bool capture = board.is_capture(move);
            const bool givescheck = board.gives_check(move);
            const bool promotion = is_promotion(move);
            
            const bool doFutility = !checked && !capture && !givescheck && !promotion;
            
            if (nodetype != PvNode && movenum >= 1 && depth <= 3 && doFutility) {
                if ((eval + FutilityMargin[depth]) <= alpha) {
                    continue;
                }
            }

            int reductions = 0;
            int extensions = 0;

			if (checked && board.see(move, board.turn()) >= 0) {
                extensions++;
            }
			
			// Late Move Reduction
            if (!capture && !checked && !givescheck && !promotion && movenum > (nodetype == PvNode ? 4 : 2) && depth >= 3) {
                reductions = std::max(0, std::min((depth / 4) + (movenum / 8) + ((nodetype != PvNode) ? 1 : -1) + ((move == info->killers[board.plies()][0] || move == info->killers[board.plies()][1]) ? -1 : 0),  depth - 2));
			}
			
			board.do_move(move);
            
			newpv.size = 0;
            
            if (movenum > 1) {
                score = -alphabeta(-alpha - 1, -alpha, depth - 1 + extensions - reductions, CutNode, board, info, newpv, nullmove);
                if (score > alpha && reductions) {
                    score = -alphabeta(-alpha - 1, -alpha, depth - 1 + extensions, (nodetype == PvNode ? CutNode : PvNode), board, info, newpv, nullmove);
                }
                if (score > alpha && score < beta) {
                    score = -alphabeta(-beta, -alpha, depth - 1 + extensions, PvNode, board, info, newpv, nullmove);
                }
            } else {
                score = -alphabeta(-beta, -alpha, depth - 1 + extensions, nodetype, board, info, newpv, nullmove);
            }
            
            board.undo_move();
            
            if (score > bestScore) {
                bestScore = score;
                if (score >= beta) {                                        
                    if (!capture && move != info->killers[board.plies()][0]) {
                        info->killers[board.plies()][1] = info->killers[board.plies()][0];
                        info->killers[board.plies()][0] = move;
                        info->history[board.piecetype(from_sq(move))][to_sq(move)] += depth * depth;
						if (pmsq != NOSQ) info->countermove[board.piecetype(pmsq)][pmsq] = move;
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
            if (checked) {
                return -MATEVALUE + board.plies();
            } else {
                return DRAWVALUE;
            }
        }
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
    
    int score, lastscore;
    
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

        if (dcount > 5 && std::abs(lastscore) < MATEVALUE) {

            int alphawindow, betawindow;
            alphawindow = betawindow = 25 - std::min(dcount / 3, 10);
        
            while (true) {
                
                int alpha = lastscore - alphawindow;
                int beta = lastscore + betawindow;
                
                score = alphabeta(alpha, beta, dcount, PvNode, board, &info, pv, true);
                
                if (score <= alpha) {
                    alphawindow *= 2;
                } else if (score >= beta) {
                    betawindow *= 2;
                } else {
                    break;
                }

                pv.clear();
                
            }
            
        } else {
            
            score = alphabeta(-INFINITE, INFINITE, dcount, PvNode, board, &info, pv, true);
            
        }
        
        end:
        
            lastscore = score;
            
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
