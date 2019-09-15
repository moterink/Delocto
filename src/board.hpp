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
    Value pst[2];
    Value material[2];
    uint64_t kingBlockers[2];
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

        inline Color turn() const { return stm; }

        inline uint64_t pieces(const unsigned int type) const             { return bitboards[type]; }
        inline uint64_t pieces(const PieceType pt, const Color color) const { return bitboards[(pt | color)]; }

        inline Color owner(const unsigned int sq) const          { return (piecetypes[sq] & 1); }
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

        inline Value material(const Color color) const { return state.material[color]; }
        inline Value pst     (const Color color) const { return state.pst[color];      }

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
        inline bool can_castle(const unsigned int flag) const;
        bool gives_check(const Move move);

        int mvvlva(const Move move) const;
        int see(const Move move, Color color) const;

        inline uint64_t minors_or_majors(const Color color) const;
        inline uint64_t all_majors() const;
        inline bool is_capture(const Move move) const;
        inline bool isDangerousPawnPush(const Move move, const Color color);
        inline uint64_t getAllOccupancy(const uint64_t squares) const;
        inline uint64_t getPieceOccupancy(const uint64_t squares, const PieceType type);
        uint64_t get_king_blockers(const Color color) const;
        uint64_t get_slider_blockers(const uint64_t sliders, const unsigned int sq) const;

        inline uint64_t gen_wpawns_attacks() const;
        inline uint64_t gen_bpawns_attacks() const;
        inline uint64_t gen_pawns_attacks(const Color color) const;
        inline uint64_t get_same_colored_squares(const unsigned int sq) const;

    private:

        StateInfo state;

        // Bitboards
        // 0 - 1:  All pieces of White/Black
        // 2 - 13: Specific pieces
        // 14:     Trash for NOPIECE
        // 15:     All pieces combined
        uint64_t bitboards[16];

        // Color to move
        Color stm;

        // Plies
        unsigned int ply = 0;

        // Piecetypes
        PieceType piecetypes[64];

        std::vector<StateInfo> states;

        // Piece type counts
        int piececounts[14];

        void clear();
        void update_bitboards();
        uint64_t get_pinned(const Color color);

        inline void hash_pawn(const Color color, const unsigned int sq);
        inline void hash_piece(const PieceType pt, const unsigned int sq);
        inline void hash_castling();
        inline void hash_turn();
        inline void hash_enPassant();
        inline void hash_material(const PieceType pt);

        void calc_keys();

        inline uint64_t pseudo_bb(const PieceType pt, const Color color, const unsigned int sq) const;

        inline bool singleSquareAttacked(uint64_t squares, const Color color) const;

        inline uint64_t all_attackers(const unsigned int sq, const uint64_t occupied) const;
        inline uint64_t color_attackers(const unsigned int sq, const uint64_t occupied, const Color color) const;
        inline uint64_t all_slider_attackers(const unsigned int sq, const uint64_t occupied) const;
        inline uint64_t color_slider_attackers(const unsigned int sq, const uint64_t occupied, const Color color) const;
        inline uint64_t color_slider_attackers(const Color color, const unsigned int sq, const uint64_t bStart, const uint64_t bEnd);

        inline bool sq_attacked(const unsigned int sq, const Color color) const;
        inline bool sq_attacked_noking(const unsigned int sq, const Color color) const;

        uint64_t getLeastValuablePiece(uint64_t attackers, const Color color, PieceType& pt) const;

        void update_check_info();

};

inline bool Board::can_castle(const unsigned int flag) const {

    return state.castling & flag;

}

inline uint64_t Board::minors_or_majors(const Color color) const {

    return bitboards[Queen(color)] | bitboards[Rook(color)] | bitboards[Bishop(color)] | bitboards[Knight(color)];

}

inline uint64_t Board::all_majors() const {

    return bitboards[WHITE_ROOK] | bitboards[BLACK_ROOK] | bitboards[WHITE_QUEEN] | bitboards[BLACK_QUEEN];

}

inline bool Board::is_capture(const Move move) const {

    return (piecetypes[to_sq(move)] != NOPIECE || move_type(move) == ENPASSANT);

}

inline bool Board::isDangerousPawnPush(const Move move, const Color color) {

    return (SQUARES[to_sq(move)] & (bitboards[Pawn(color)] & DangerousPawnZone[color]));

}

inline uint64_t Board::getAllOccupancy(const uint64_t squares) const {

    return bitboards[ALLPIECES] & squares;

}

inline uint64_t Board::getPieceOccupancy(const uint64_t squares, const PieceType type) {

    return bitboards[type] & squares;

}

inline uint64_t Board::get_king_blockers(const Color color) const {

    return state.kingBlockers[color];

}

inline uint64_t Board::get_same_colored_squares(const unsigned int sq) const {

    return (SQUARES[sq] & WHITE_SQUARES) ? WHITE_SQUARES : BLACK_SQUARES;

}

inline bool Board::singleSquareAttacked(uint64_t squares, const Color color) const {

    while (squares) {

        if (sq_attacked(pop_lsb(squares), color)) {
            return true;
        }

    }

    return false;

}

// Hash pawn in/out of key
inline void Board::hash_pawn(const Color color, const unsigned int sq) {

    state.pawnKey ^= pawnHashKeys[color][sq];

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

inline uint64_t Board::color_attackers(const unsigned int sq, const uint64_t occupied, const Color color) const {

    return ((AttackBitboards[Pawn(!color)][sq]     & bitboards[Pawn(color)])
          | (AttackBitboards[KNIGHT][sq]          & bitboards[Knight(color)])
          | (generateBishopMoves(sq, occupied, 0) & (bitboards[Bishop(color)] | bitboards[Queen(color)]))
          | (generateRookMoves  (sq, occupied, 0) & (bitboards[Rook(color)]   | bitboards[Queen(color)]))
          | (AttackBitboards[KING][sq]            & bitboards[King(color)]));

}

inline uint64_t Board::all_slider_attackers(const unsigned int sq, const uint64_t occupied) const {

    return (generateBishopMoves(sq, occupied, 0) & (bitboards[WHITE_BISHOP] | bitboards[BLACK_BISHOP] | bitboards[WHITE_QUEEN] | bitboards[BLACK_QUEEN]))
         | (generateRookMoves(sq, occupied, 0)   & (bitboards[WHITE_ROOK]   | bitboards[BLACK_ROOK]   | bitboards[WHITE_QUEEN] | bitboards[BLACK_QUEEN]));

}

inline uint64_t Board::color_slider_attackers(const unsigned int sq, const uint64_t occupied, const Color color) const {

    return (generateBishopMoves(sq, occupied, 0) & (bitboards[Bishop(color)] | bitboards[Queen(color)]))
         | (generateRookMoves(sq, occupied, 0)   & (bitboards[Rook(color)]   | bitboards[Queen(color)]));

}

inline uint64_t Board::color_slider_attackers(const Color color, const unsigned int sq, const uint64_t bStart, const uint64_t bEnd) {

    const uint64_t occupied = (bitboards[ALLPIECES] ^ bStart) | bEnd;

    uint64_t xrays = ((generateBishopMoves(sq, occupied, 0) & (bitboards[Bishop(color)] | bitboards[Queen(color)]))
                    | (generateRookMoves  (sq, occupied, 0) & (bitboards[Rook(color)]   | bitboards[Queen(color)])));

    return (xrays & ~bStart);

}

// Check if a square is attacked by a given color
inline bool Board::sq_attacked(const unsigned int sq, const Color color) const {

    return ((AttackBitboards[Pawn(!color)][sq]                 & bitboards[Pawn(color)])
          | (AttackBitboards[KNIGHT][sq]                      & bitboards[Knight(color)])
          | (generateBishopMoves(sq, bitboards[ALLPIECES], 0) & (bitboards[Bishop(color)] | bitboards[Queen(color)]))
          | (generateRookMoves  (sq, bitboards[ALLPIECES], 0) & (bitboards[Rook(color)] | bitboards[Queen(color)]))
          | (AttackBitboards[KING][sq]                        & bitboards[King(color)])) != 0;

}

// Check if a square is attacked by a given color without the enemy king on the board
inline bool Board::sq_attacked_noking(const unsigned int sq, const Color color) const {

    return ((AttackBitboards[Pawn(!color)][sq]                                            & bitboards[Pawn(color)])
          | (AttackBitboards[KNIGHT][sq]                                                 & bitboards[Knight(color)])
          | (generateBishopMoves(sq, (bitboards[ALLPIECES] ^ bitboards[King(!color)]), 0) & (bitboards[Bishop(color)] | bitboards[Queen(color)]))
          | (generateRookMoves  (sq, (bitboards[ALLPIECES] ^ bitboards[King(!color)]), 0) & (bitboards[Rook(color)] | bitboards[Queen(color)]))
          | (AttackBitboards[KING][sq]                                                   & bitboards[King(color)])) != 0;

}

inline uint64_t Board::gen_wpawns_attacks() const {

    return ((bitboards[WHITE_PAWN] & ~FILE_A) << 9) | ((bitboards[WHITE_PAWN] & ~FILE_H) << 7);

}

inline uint64_t Board::gen_bpawns_attacks() const {

    return ((bitboards[BLACK_PAWN] & ~FILE_A) >> 7) | ((bitboards[BLACK_PAWN] & ~FILE_H) >> 9);

}

inline uint64_t Board::gen_pawns_attacks(const Color color) const {

    return (color == WHITE) ? gen_wpawns_attacks() : gen_bpawns_attacks();

}

// Get a bitboards with all pseudo-legal destination squares for a slider on the given square
inline uint64_t slider_pseudo_bb(const PieceType pt, const unsigned int sq, const uint64_t occupied, const uint64_t friendly) {

    return (pt == BISHOP) ? generateBishopMoves(sq, occupied, friendly) : (pt == ROOK) ? generateRookMoves(sq, occupied, friendly) : generateQueenMoves(sq, occupied, friendly);

}

// Get a bitboard with all pseudo-legal destination squares for a piece on the given square
inline uint64_t Board::pseudo_bb(const PieceType pt, const Color color, const unsigned int sq) const {

    return (pt == BISHOP || pt == ROOK || pt == QUEEN) ? slider_pseudo_bb(pt, sq, bitboards[ALLPIECES], bitboards[color]) : AttackBitboards[pt][sq] & ~bitboards[color];

}

#endif
