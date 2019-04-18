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
#include <algorithm>
#include <ctype.h>
#include "board.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "hashkeys.hpp"

static const unsigned int CastleMask[64] = {

    7, 15, 15, 15, 3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14

};

void Board::updateSideBitboards() {

    bitboards[WHITE]     = bitboards[WHITE_PAWN] | bitboards[WHITE_KNIGHT] | bitboards[WHITE_BISHOP] | bitboards[WHITE_ROOK] | bitboards[WHITE_QUEEN] | bitboards[WHITE_KING];
    bitboards[BLACK]     = bitboards[BLACK_PAWN] | bitboards[BLACK_KNIGHT] | bitboards[BLACK_BISHOP] | bitboards[BLACK_ROOK] | bitboards[BLACK_QUEEN] | bitboards[BLACK_KING];
    bitboards[ALLPIECES] = bitboards[WHITE] | bitboards[BLACK];

}

// Reset board
void Board::clear() {

    states.clear();
    moves.clear();

    bitboards[WHITE] = 0;
    bitboards[BLACK] = 0;

    bitboards[WHITE_PAWN] = 0;
    bitboards[WHITE_KNIGHT] = 0;
    bitboards[WHITE_BISHOP] = 0;
    bitboards[WHITE_ROOK] = 0;
    bitboards[WHITE_QUEEN] = 0;
    bitboards[WHITE_KING] = 0;

    bitboards[BLACK_PAWN] = 0;
    bitboards[BLACK_KNIGHT] = 0;
    bitboards[BLACK_BISHOP] = 0;
    bitboards[BLACK_ROOK] = 0;
    bitboards[BLACK_QUEEN] = 0;
    bitboards[BLACK_KING] = 0;

    for (int pcount = 0; pcount < 64; pcount++) {
        piecetypes[pcount] = NOPIECE;
    }

    std::fill(piececounts, piececounts+12, 0);

    state.enPassant = NOSQ;
    state.castling = 0;
    state.fiftyMoves = 0;
    state.material[WHITE] = S(0, 0);
    state.material[BLACK] = S(0, 0);
    state.captured = NOPIECE;
    state.pst[WHITE] = S(0, 0);
    state.pst[BLACK] = S(0, 0);
    state.pinned = 0;
    state.checkers = 0;

    ply = 0;

    stm = WHITE;

}

// Calculate hash, pawn and material key
void Board::calc_keys() {

    state.hashKey = 0;
    state.pawnKey = 0;
    state.materialKey = 0;

    uint64_t temp = pieces(ALLPIECES);

    while (temp) {

        const unsigned int sq = pop_lsb(temp);
        const PieceType pt    = piecetypes[sq];

        hash_piece(pt, sq);

        if (pt == WHITE_PAWN || pt == BLACK_PAWN) {
            hash_pawn(owner(sq), sq);
        }

    }

    for (unsigned int pt = WHITE_PAWN; pt <= BLACK_QUEEN; pt++) {
        hash_material(pt);
    }

    hash_enPassant();
    hash_castling();
    hash_turn();

}

uint64_t Board::get_pinned(const Side side) {

    const unsigned int ksq = lsb_index(bitboards[King(side)]);
    uint64_t pinnedPieces = 0;
    uint64_t pinners = (AttackBitboards[BISHOP][ksq] & (bitboards[Bishop(!side)] | bitboards[Queen(!side)])) | (AttackBitboards[ROOK][ksq] & (bitboards[Rook(!side)] | bitboards[Queen(!side)]));

    while (pinners) {

        const unsigned int sq = pop_lsb(pinners);
        const uint64_t pin    = RayTable[sq][ksq];
        const uint64_t pinned = pin & (bitboards[ALLPIECES] ^ SQUARES[ksq]);

        if (popcount(pinned) == 1) {
            pinnedPieces |= pinned;
        }

    }

    return pinnedPieces;

}

int Board::mvvlva(const Move move) const {

    if (is_ep(move)) {
        return MvvLvaVictim[0] - MvvLvaAttacker[0];
    }

    int value = 0;

    if (is_promotion(move)) {
        value += MvvLvaVictim[pt_index(prom_piecetype(move_type(move), 0))];
    }

    value += MvvLvaVictim[pt_index(piecetypes[to_sq(move)])] - MvvLvaAttacker[pt_index(piecetypes[from_sq(move)])];

    return value;

}

// Get board from fen string
void Board::set_fen(std::string fen) {

    clear();

    std::string nums = "12345678";

    int index = -1;
    unsigned int findex = 0;

    while (fen[findex] != ' ') {

        index++;

        Side side = WHITE;
        PieceType type = NOPIECE;

        switch (fen[findex]) {

            case 'P': { side = WHITE; type = WHITE_PAWN; break; }
            case 'N': { side = WHITE; type = WHITE_KNIGHT; break; }
            case 'B': { side = WHITE; type = WHITE_BISHOP; break; }
            case 'R': { side = WHITE; type = WHITE_ROOK; break; }
            case 'Q': { side = WHITE; type = WHITE_QUEEN; break; }
            case 'K': { side = WHITE; type = WHITE_KING; break; }
            case 'p': { side = BLACK; type = BLACK_PAWN; break; }
            case 'n': { side = BLACK; type = BLACK_KNIGHT; break; }
            case 'b': { side = BLACK; type = BLACK_BISHOP; break; }
            case 'r': { side = BLACK; type = BLACK_ROOK; break; }
            case 'q': { side = BLACK; type = BLACK_QUEEN; break; }
            case 'k': { side = BLACK; type = BLACK_KING; break; }

        }

        if (fen[findex] == '/') {
            index--;
        } else if (nums.find(fen[findex]) != std::string::npos) {
            int pad = fen[findex] - '0';
            pad--;
            index += pad;
        } else {
            bitboards[side] |= (1ULL << (63 - index));
            bitboards[type] |= (1ULL << (63 - index));
            state.pst[side] += Pst[type][index];
            piecetypes[index] = type;
            state.material[side] += Material[type];
            piececounts[type]++;
        }

        ++findex;

    }

    ++findex;
    stm = (fen[findex] == 'w') ? WHITE : BLACK;
    findex += 2;

    while (fen[findex] != ' ') {

        switch (fen[findex]) {

            case 'K': { state.castling |= WKCASFLAG; break; }
            case 'Q': { state.castling |= WQCASFLAG; break; }
            case 'k': { state.castling |= BKCASFLAG; break; }
            case 'q': { state.castling |= BQCASFLAG; break; }

        }

        ++findex;

    }

    ++findex;

    state.enPassant = (fen[findex] != '-') ? square(fen[findex] - 'a', 8 - (fen[findex + 1] - '0')) : NOSQ;

    findex += 2;

    if (state.enPassant != NOSQ) {
        findex++;
    }

    if (fen.size() >= findex && isdigit(fen[findex])) {
        std::string cut = fen.substr(findex);
        state.fiftyMoves = std::stoi(cut.substr(0, cut.find(" ")));
    } else {
        state.fiftyMoves = 0;
    }

    updateSideBitboards();

    state.pinned   = get_pinned(stm);
    state.checkers = side_attackers(lsb_index(bitboards[King(stm)]), bitboards[ALLPIECES], !stm);

    calc_keys();

}

// Print board to screen for debuging purposals
void Board::print() const {

    char labels[15] = { ' ', ' ', 'P', 'p', 'N', 'n', 'B', 'b', 'R', 'r', 'Q', 'q', 'K', 'k', '.' };
    unsigned int r  = 0;
    unsigned int sq = 0;

    while (sq != 64) {

        if (rank(sq) != r) {
            std::cout << std::endl;
            r++;
        }

        std::cout << labels[piecetypes[sq]] << ' ';
        sq++;
    }

    std::cout << std::endl << std::endl;

}

bool Board::do_move(const Move move) {

    assert(move != NOMOVE);

    const int fromsq      = from_sq(move);
    const int tosq        = to_sq(move);
    const MoveType  mtype = move_type(move);
    const PieceType ftype = piecetypes[fromsq];
    const PieceType ttype = piecetypes[tosq];

    states.push_back(state);
    moves.push_back(move);

    state.captured = ttype;
    state.checkers = 0;
    hash_enPassant(); // hashEnPassant checks for NOSQ
    state.enPassant = NOSQ;
    state.fiftyMoves++;

    piecetypes[fromsq] = NOPIECE;
    piecetypes[tosq] = ftype;

    state.pst[stm] -= Pst[ftype][fromsq];
    state.pst[stm] += Pst[ftype][tosq];

    bitboards[ftype] ^= SQUARES[fromsq];
    bitboards[ftype] |= SQUARES[tosq];
    bitboards[stm] ^= SQUARES[fromsq];
    bitboards[stm] |= SQUARES[tosq];

    hash_piece(ftype, fromsq);
    hash_piece(ftype, tosq);

    hash_castling(); // XOR castling out

    // NOTE: no need to check if piece on square -> pieces[NOPIECE] is trash

    if (ttype != NOPIECE) {

        bitboards[ttype] ^= SQUARES[tosq];
        bitboards[!stm] ^= SQUARES[tosq];

        state.material[!stm] -= Material[ttype];
        state.pst[!stm] -= Pst[ttype][tosq];

        hash_piece(ttype, tosq);
        hash_material(ttype);
        piececounts[ttype]--;
        hash_material(ttype);

        state.fiftyMoves = 0;

        if (type(ttype) == PAWN) {
            hash_pawn(!stm, tosq);
        }
    }

    switch(mtype) {

        case NORMAL:

            {

                // Update enpassant square
                if (type(ftype) == PAWN) {
                    state.fiftyMoves = 0;
                    hash_pawn(stm, fromsq);
                    hash_pawn(stm, tosq);
                    if (std::abs(fromsq - tosq) == 16) {
                        if (attackingPawns[!stm][fromsq + DIRECTIONS[stm][UP]] & bitboards[Pawn(!stm)]) {
                            state.enPassant = fromsq + DIRECTIONS[stm][UP];
                            hash_enPassant();
                        }
                    }
                }

                state.castling &= CastleMask[fromsq];

            }
            break;

        case CASTLING:

            {

                const unsigned int rtosq   = tosq + ((tosq == G1 || tosq == G8) ? -1 :  1);
                const unsigned int rfromsq = tosq + ((tosq == G1 || tosq == G8) ?  1 : -2);
                const PieceType rook = Rook(stm);

                bitboards[rook] ^= SQUARES[rfromsq];
                bitboards[rook] |= SQUARES[rtosq];
                bitboards[stm] ^= SQUARES[rfromsq];
                bitboards[stm] |= SQUARES[rtosq];

                state.pst[stm] -= Pst[rook][rfromsq];
                state.pst[stm] += Pst[rook][rtosq];

                hash_piece(rook, rfromsq);
                hash_piece(rook, rtosq);

                piecetypes[rfromsq] = NOPIECE;
                piecetypes[rtosq] = rook;

                state.castling &= CastleMask[fromsq];

            }
            break;

        case ENPASSANT:

            {

                const unsigned int capsq = tosq + DIRECTIONS[stm][DOWN];
                const PieceType pawn = Pawn(!stm);

                // NOTE: do not assign pawn to state.captured!!!

                bitboards[pawn] ^= SQUARES[capsq];
                bitboards[!stm] ^= SQUARES[capsq];

                state.material[!stm] -= Material[pawn];
                state.pst[!stm] -= Pst[pawn][capsq];

                hash_pawn(stm, fromsq);
                hash_pawn(stm, tosq);

                hash_piece(pawn, capsq);
                hash_pawn(!stm, capsq);
                hash_material(pawn);
                piececounts[pawn]--;
                hash_material(pawn);

                piecetypes[capsq] = NOPIECE;

                state.fiftyMoves = 0;

            }
            break;

        default:

            {

                const PieceType ptype = prom_piecetype(mtype, stm);

                // Change pawn to promotion piece
                bitboards[ftype] ^= SQUARES[tosq];
                bitboards[ptype] |= SQUARES[tosq];

                state.material[stm] -= Material[ftype];
                state.material[stm] += Material[ptype];

                state.pst[stm] -= Pst[ftype][tosq];
                state.pst[stm] += Pst[ptype][tosq];

                hash_piece(Pawn(stm), tosq);
                hash_piece(ptype, tosq);
                hash_pawn(stm, fromsq);
                hash_material(Pawn(stm));
                piececounts[Pawn(stm)]--;
                piececounts[ptype]++;
                hash_material(ptype);

                piecetypes[tosq] = ptype;

                state.fiftyMoves = 0;

            }
            break;

    }

    hash_castling(); // hash castling back in

    hash_turn();
    stm = !stm;
    hash_turn();

    bitboards[ALLPIECES] = bitboards[WHITE] | bitboards[BLACK];

    state.pinned   = get_pinned(stm);
    state.checkers = side_attackers(lsb_index(bitboards[King(stm)]), bitboards[ALLPIECES], !stm);

    ply++;

    return true;

}

void Board::undo_move() {

    assert(move != NOMOVE);

    const Move move = moves.back();

    moves.pop_back();

    const unsigned int fromsq = from_sq(move);
    const unsigned int tosq   = to_sq(move);
    const MoveType type       = move_type(move);
    const PieceType ftype     = piecetypes[tosq];

    piecetypes[fromsq] = ftype;
    piecetypes[tosq] = state.captured;

    stm = !stm;

    bitboards[ftype] ^= SQUARES[tosq];
    bitboards[ftype] |= SQUARES[fromsq];
    bitboards[stm] ^= SQUARES[tosq];
    bitboards[stm] |= SQUARES[fromsq];

    if (state.captured != NOPIECE) {
        bitboards[state.captured] |= SQUARES[tosq];
        bitboards[!stm] |= SQUARES[tosq];
        piececounts[state.captured]++;
    }

    switch(type) {

        case NORMAL: break;
        case CASTLING:

            {

                const unsigned int rtosq   = tosq + ((tosq == G1 || tosq == G8) ? -1 :  1);
                const unsigned int rfromsq = tosq + ((tosq == G1 || tosq == G8) ?  1 : -2);
                const PieceType rook = Rook(stm);

                bitboards[rook] ^= SQUARES[rtosq];
                bitboards[rook] |= SQUARES[rfromsq];
                bitboards[stm] ^= SQUARES[rtosq];
                bitboards[stm] |= SQUARES[rfromsq];

                piecetypes[rtosq] = NOPIECE;
                piecetypes[rfromsq] = rook;

            }
            break;

        case ENPASSANT:

            {

                const unsigned int capsq = tosq + DIRECTIONS[stm][DOWN];
                const PieceType pawn = Pawn(!stm);

                bitboards[pawn] |= SQUARES[capsq];
                bitboards[!stm] |= SQUARES[capsq];
                piececounts[pawn]++;

                piecetypes[capsq] = pawn;

            }
            break;

        default:

            {

                const PieceType ptype = Pawn(stm);

                // Change pawn to promotion piece
                bitboards[ftype] ^= SQUARES[fromsq];
                piececounts[ftype]--;
                bitboards[ptype] |= SQUARES[fromsq];
                piececounts[ptype]++;

                piecetypes[fromsq] = ptype;

            }
            break;

    }

    bitboards[ALLPIECES] = bitboards[WHITE] | bitboards[BLACK];

    ply--;

    state = states.back();
    states.pop_back();

}

// Make a null move on the board
void Board::do_nullmove() {

    states.push_back(state);

    hash_enPassant(); // hashEnPassant checks for NOSQ
    state.enPassant = NOSQ;

    hash_turn();
    stm = !stm;
    hash_turn();

    state.pinned   = get_pinned(stm);
    state.checkers = side_attackers(lsb_index(bitboards[King(stm)]), bitboards[ALLPIECES], !stm);

    ply++;

}

// Unmake a null move on the board
void Board::undo_nullmove() {

    stm = !stm;

    ply--;

    state = states.back();

    states.pop_back();

}

// Checks for threefold repetition and 50 move rule. Stalemate is settled by search
bool Board::checkDraw() {

    unsigned int count = 0;
    for (unsigned int index = 0; index < states.size();  index++) {
        if (states[index].hashKey == state.hashKey) {
            count++;
        }
    }

    if (count >= 2) {
        return true;
    }

    if (state.fiftyMoves >= 100) {
        return true;
    }

    return false;

}

bool Board::is_valid(const Move move) const {

    const unsigned int fromsq = from_sq(move);
    const unsigned int tosq   = to_sq(move);

    if (piecetypes[fromsq] == NOPIECE || owner(fromsq) != stm || (SQUARES[tosq] & bitboards[stm])) {
        return false;
    }

    const MoveType mtype = move_type(move);

    if (mtype == NORMAL) {
        if (SQUARES[fromsq] & bitboards[Pawn(stm)]) {
            // Push to 8th rank invalid -> would be promotion, so mtype can't be NORMAL
            if (relative_rank(stm, tosq) == 7) {
                return false;
            }
            if (!((SQUARES[tosq] & AttackBitboards[Pawn(stm)][fromsq]) & bitboards[!stm])
               && !((fromsq + DIRECTIONS[stm][UP] == tosq) && piecetypes[tosq] == NOPIECE)
               && !(relative_rank(stm, fromsq) == 1 && (fromsq + 2 * DIRECTIONS[stm][UP] == tosq) && (piecetypes[fromsq + DIRECTIONS[stm][UP]] == NOPIECE) && (piecetypes[fromsq + 2 * DIRECTIONS[stm][UP]] == NOPIECE))) {
                    return false;
            }
        } else if (!(SQUARES[tosq] & pseudo_bb(type(piecetypes[fromsq]), stm, fromsq))) {
            return false;
        }
    } else {
        if (mtype == CASTLING) {
            if (is_castling_valid(castleByKingpos[tosq]) == false) {
                return false;
            }
        }

        if (is_promotion(move)) {
            if (!(SQUARES[fromsq] & bitboards[Pawn(stm)]) || (!(SQUARES[tosq] & ((stm == WHITE) ? RANK_1 : RANK_8))) || !(SQUARES[tosq] & generatePawnMoves(stm, fromsq, bitboards[ALLPIECES], bitboards[!stm]))) {
                return false;
            }
        }

        if (mtype == ENPASSANT) {
            if (tosq != state.enPassant || !(SQUARES[fromsq] & bitboards[Pawn(stm)])) {
                return false;
            }
        }
    }

    if (checkers()) {        
        if (SQUARES[fromsq] != bitboards[King(stm)]) {

            const unsigned int ksq = lsb_index(bitboards[King(stm)]);

            if (popcount(checkers()) >= 2) {
                return false;
            }

            if (!(((RayTable[lsb_index(checkers())][ksq] | checkers())) & SQUARES[tosq])) {
                return false;
            }

        } else if (side_attackers(tosq, (bitboards[ALLPIECES] ^ SQUARES[fromsq]), !stm) & bitboards[!stm]) {
            return false;
        }
    }

    return true;

}
