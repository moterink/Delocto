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

#ifndef BOARD_H
#define BOARD_H

#include "types.hpp"
#include "move.hpp"
#include "hashkeys.hpp"
#include "movegen.hpp"
#include "bitboards.hpp"

#define STARTFEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

typedef struct {

    unsigned int castling = 0;
    unsigned int enPassant = NOSQ;
    unsigned int fiftyMoves = 0;
    PieceType captured = NOPIECE;
    Score pst[2];
    Score material[2];
    uint64_t pinned = 0;
    uint64_t checkers = 0;
    uint64_t hashKey = 0;
    uint64_t pawnKey = 0;
    uint64_t materialKey = 0;

}StateInfo;

static const uint64_t DangerousPawnZone[2] = { 0xffffffff00000000, 0xffffffff };
static const int MvvLvaVictim[5] = { 100, 200, 300, 400, 500 };
static const int MvvLvaAttacker[6] = { 1, 2, 3, 4, 5, 0 };

// Board class: Stores all necessary information e.g bitboards and piecelists
class Board {

    public:

        std::vector<Move> moves;

        inline Side turn() const { return stm; }

        inline uint64_t pieces(const unsigned int type) const             { return bitboards[type]; }
        inline uint64_t pieces(const PieceType pt, const Side side) const { return bitboards[(pt | side)]; }

        inline Side owner(const unsigned int sq) const          { return (piecetypes[sq] & 1); }
        inline PieceType piecetype(const unsigned int sq) const { return piecetypes[sq]; }

        inline uint64_t checkers() const         { return state.checkers; }
        inline uint64_t pinned()   const         { return state.pinned;   }
        inline unsigned int castleRights() const { return state.castling; }

        inline unsigned int enPassant() const { return state.enPassant; }

        inline uint64_t hashkey() const     { return state.hashKey; }
        inline uint64_t materialkey() const { return state.materialKey; }
        inline uint64_t pawnkey() const     { return state.pawnKey; }

        inline unsigned int plies() const { return ply; }
        inline void reset_plies() { ply = 0; }

        inline Score material(const Side side) const { return state.material[side]; }
        inline Score pst     (const Side side) const { return state.pst[side];      }

        inline const unsigned int piececount(const PieceType pt) const { return piececounts[pt]; }
        inline const unsigned int scale() const { return std::max(((24 - 4 * (piececounts[WHITE_QUEEN] + piececounts[BLACK_QUEEN]) - 2 * (piececounts[WHITE_ROOK] + piececounts[BLACK_QUEEN]) - (piececounts[WHITE_BISHOP] + piececounts[BLACK_BISHOP]) - (piececounts[WHITE_KNIGHT] + piececounts[BLACK_KNIGHT])) * 256 + 12) / 24, 0); }

        void set_fen(std::string fen);
        void print() const;

        bool do_move(const Move move);
        void undo_move();
        void do_nullmove();
        void undo_nullmove();

        bool checkDraw();
        bool checkMaterialDraw(const unsigned int pieceCount) const;

        bool is_valid(const Move move) const;
        bool is_legal(const Move move) const;
        bool is_castling_valid(const unsigned int flag) const;
        bool gives_check(const Move move);

        int mvvlva(const Move move) const;
        int see(const Move move, Side side) const;

        inline uint64_t minors_or_majors(const Side side) const;
        inline bool is_capture(const Move move) const;
        inline bool isDangerousPawnPush(const Move move, const Side side);
        inline uint64_t getAllOccupancy(const uint64_t squares) const;
        inline uint64_t getPieceOccupancy(const uint64_t squares, const PieceType type);
        uint64_t get_king_blockers(const Side side) const;

        inline uint64_t gen_wpawns_attacks() const;
        inline uint64_t gen_bpawns_attacks() const;
        inline uint64_t gen_pawns_attacks(const Side side) const;
        inline uint64_t get_same_colored_squares(const unsigned int sq) const;

    private:

        StateInfo state;

        // Bitboards
        // 0 - 1:  All pieces of White/Black
        // 2 - 13: Specific pieces
        // 14:     Trash for NOPIECE
        // 15:     All pieces combined
        uint64_t bitboards[16];

        // Side to move
        Side stm;

        // Plies
        unsigned int ply = 0;

        // Piecetypes
        PieceType piecetypes[64];

        std::vector<StateInfo> states;

        // Piece type counts
        int piececounts[14];

        void clear();
        void updateSideBitboards();
        uint64_t get_pinned(const Side side);

        inline void hash_pawn(const Side side, const unsigned int sq);
        inline void hash_piece(const PieceType pt, const unsigned int sq);
        inline void hash_castling();
        inline void hash_turn();
        inline void hash_enPassant();
        inline void hash_material(const PieceType pt);

        void calc_keys();

        inline uint64_t pseudo_bb(const PieceType pt, const Side side, const unsigned int sq) const;

        inline bool singleSquareAttacked(uint64_t squares, const Side side) const;

        inline uint64_t all_attackers(const unsigned int sq, const uint64_t occupied) const;
        inline uint64_t side_attackers(const unsigned int sq, const uint64_t occupied, const Side side) const;
        inline uint64_t all_slider_attackers(const unsigned int sq, const uint64_t occupied) const;
        inline uint64_t side_slider_attackers(const unsigned int sq, const uint64_t occupied, const Side side) const;
        inline uint64_t side_slider_attackers(const Side side, const unsigned int sq, const uint64_t bStart, const uint64_t bEnd);

        inline bool sq_attacked(const unsigned int sq, const Side side) const;
        inline bool sq_attacked_noking(const unsigned int sq, const Side side) const;

        uint64_t getLeastValuablePiece(uint64_t attackers, const Side side, PieceType& pt) const;

        void update_check_info();

};

inline uint64_t Board::minors_or_majors(const Side side) const {

    return bitboards[Queen(side)] | bitboards[Rook(side)] | bitboards[Bishop(side)] | bitboards[Knight(side)];

}

inline bool Board::is_capture(const Move move) const {

    return (piecetypes[to_sq(move)] != NOPIECE || move_type(move) == ENPASSANT);

}

inline bool Board::isDangerousPawnPush(const Move move, const Side side) {

    return (SQUARES[to_sq(move)] & (bitboards[Pawn(side)] & DangerousPawnZone[side]));

}

inline uint64_t Board::getAllOccupancy(const uint64_t squares) const {

    return bitboards[ALLPIECES] & squares;

}

inline uint64_t Board::getPieceOccupancy(const uint64_t squares, const PieceType type) {

    return bitboards[type] & squares;

}

inline uint64_t Board::get_same_colored_squares(const unsigned int sq) const {

    return (SQUARES[sq] & WHITE_SQUARES) ? WHITE_SQUARES : BLACK_SQUARES;

}

inline bool Board::singleSquareAttacked(uint64_t squares, const Side side) const {

    while (squares) {

        if (sq_attacked(pop_lsb(squares), side)) {
            return true;
        }

    }

    return false;

}

// Hash pawn in/out of key
inline void Board::hash_pawn(const Side side, const unsigned int sq) {

    state.pawnKey ^= pawnHashKeys[side][sq];

}

// Hash piece in/out zobrist key
inline void Board::hash_piece(const PieceType pt, const unsigned int sq) {

    state.hashKey ^= pieceHashKeys[pt][sq];

}

// Hash castling in/out zobrist key
inline void Board::hash_castling() {

    state.hashKey ^= castlingHashKeys[state.castling];

}

// Hash turn in/out zobrist key
inline void Board::hash_turn() {

    state.hashKey ^= turnHashKeys[stm];

}

// Hash enPassant in/out zobrist key
inline void Board::hash_enPassant() {

    state.hashKey ^= (state.enPassant != NOSQ) ? enPassantHashKeys[file(state.enPassant)] : 0;

}

// Hash material
inline void Board::hash_material(const PieceType pt) {

    state.materialKey ^= materialHashKeys[pt][piececounts[pt]];

}

// getAttackers
inline uint64_t Board::all_attackers(const unsigned int sq, const uint64_t occupied) const {

    return  (AttackBitboards[WHITE_PAWN][sq]      & bitboards[BLACK_PAWN])
          | (AttackBitboards[BLACK_PAWN][sq]      & bitboards[WHITE_PAWN])
          | (AttackBitboards[KNIGHT][sq]          & (bitboards[WHITE_KNIGHT] | bitboards[BLACK_KNIGHT]))
          | (generateBishopMoves(sq, occupied, 0) & (bitboards[WHITE_BISHOP] | bitboards[BLACK_BISHOP] | bitboards[WHITE_QUEEN] | bitboards[BLACK_QUEEN]))
          | (generateRookMoves(sq, occupied, 0)   & (bitboards[WHITE_ROOK]   | bitboards[BLACK_ROOK]   | bitboards[WHITE_QUEEN] | bitboards[BLACK_QUEEN]))
          | (AttackBitboards[KING][sq]            & (bitboards[WHITE_KING]   | bitboards[BLACK_KING]));

}

// getSideAttackers
inline uint64_t Board::side_attackers(const unsigned int sq, const uint64_t occupied, const Side side) const {

    return ((AttackBitboards[Pawn(!side)][sq]     & bitboards[Pawn(side)])
          | (AttackBitboards[KNIGHT][sq]          & bitboards[Knight(side)])
          | (generateBishopMoves(sq, occupied, 0) & (bitboards[Bishop(side)] | bitboards[Queen(side)]))
          | (generateRookMoves  (sq, occupied, 0) & (bitboards[Rook(side)]   | bitboards[Queen(side)]))
          | (AttackBitboards[KING][sq]            & bitboards[King(side)]));

}

// getXrayAttackers
inline uint64_t Board::all_slider_attackers(const unsigned int sq, const uint64_t occupied) const {

    return (generateBishopMoves(sq, occupied, 0) & (bitboards[WHITE_BISHOP] | bitboards[BLACK_BISHOP] | bitboards[WHITE_QUEEN] | bitboards[BLACK_QUEEN]))
         | (generateRookMoves(sq, occupied, 0)   & (bitboards[WHITE_ROOK]   | bitboards[BLACK_ROOK]   | bitboards[WHITE_QUEEN] | bitboards[BLACK_QUEEN]));

}

// getSideXrayAttackers
inline uint64_t Board::side_slider_attackers(const unsigned int sq, const uint64_t occupied, const Side side) const {

    return (generateBishopMoves(sq, occupied, 0) & (bitboards[Bishop(side)] | bitboards[Queen(side)]))
         | (generateRookMoves(sq, occupied, 0)   & (bitboards[Rook(side)]   | bitboards[Queen(side)]));

}

// getXrayPieces
inline uint64_t Board::side_slider_attackers(const Side side, const unsigned int sq, const uint64_t bStart, const uint64_t bEnd) {

    const uint64_t occupied = (bitboards[ALLPIECES] ^ bStart) | bEnd;

    uint64_t xrays = ((generateBishopMoves(sq, occupied, 0) & (bitboards[Bishop(side)] | bitboards[Queen(side)]))
                    | (generateRookMoves  (sq, occupied, 0) & (bitboards[Rook(side)]   | bitboards[Queen(side)])));

    return (xrays & ~bStart);

}

// Check if a square is attacked by a given side
inline bool Board::sq_attacked(const unsigned int sq, const Side side) const {

    return ((AttackBitboards[Pawn(!side)][sq]                 & bitboards[Pawn(side)])
          | (AttackBitboards[KNIGHT][sq]                      & bitboards[Knight(side)])
          | (generateBishopMoves(sq, bitboards[ALLPIECES], 0) & (bitboards[Bishop(side)] | bitboards[Queen(side)]))
          | (generateRookMoves  (sq, bitboards[ALLPIECES], 0) & (bitboards[Rook(side)] | bitboards[Queen(side)]))
          | (AttackBitboards[KING][sq]                        & bitboards[King(side)])) != 0;

}

// Check if a square is attacked by a given side without the enemy king on the board
inline bool Board::sq_attacked_noking(const unsigned int sq, const Side side) const {

    return ((AttackBitboards[Pawn(!side)][sq]                                            & bitboards[Pawn(side)])
          | (AttackBitboards[KNIGHT][sq]                                                 & bitboards[Knight(side)])
          | (generateBishopMoves(sq, (bitboards[ALLPIECES] ^ bitboards[King(!side)]), 0) & (bitboards[Bishop(side)] | bitboards[Queen(side)]))
          | (generateRookMoves  (sq, (bitboards[ALLPIECES] ^ bitboards[King(!side)]), 0) & (bitboards[Rook(side)] | bitboards[Queen(side)]))
          | (AttackBitboards[KING][sq]                                                   & bitboards[King(side)])) != 0;

}

inline uint64_t Board::gen_wpawns_attacks() const {

    return ((bitboards[WHITE_PAWN] & ~FILE_A) << 9) | ((bitboards[WHITE_PAWN] & ~FILE_H) << 7);

}

inline uint64_t Board::gen_bpawns_attacks() const {

    return ((bitboards[BLACK_PAWN] & ~FILE_A) >> 7) | ((bitboards[BLACK_PAWN] & ~FILE_H) >> 9);

}

inline uint64_t Board::gen_pawns_attacks(const Side side) const {

    return (side == WHITE) ? gen_wpawns_attacks() : gen_bpawns_attacks();

}

// Get a bitboards with all pseudo-legal destination squares for a slider on the given square
inline uint64_t slider_pseudo_bb(const PieceType pt, const unsigned int sq, const uint64_t occupied, const uint64_t friendly) {

    return (pt == BISHOP) ? generateBishopMoves(sq, occupied, friendly) : (pt == ROOK) ? generateRookMoves(sq, occupied, friendly) : generateQueenMoves(sq, occupied, friendly);

}

// Get a bitboard with all pseudo-legal destination squares for a piece on the given square
inline uint64_t Board::pseudo_bb(const PieceType pt, const Side side, const unsigned int sq) const {

    return (pt == BISHOP || pt == ROOK || pt == QUEEN) ? slider_pseudo_bb(pt, sq, bitboards[ALLPIECES], bitboards[side]) : AttackBitboards[pt][sq] & ~bitboards[side];

}

#endif
