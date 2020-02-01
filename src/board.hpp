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
    unsigned int enPassant = SQUARE_NONE;
    unsigned int fiftyMoves = 0;
    Piecetype captured = PIECE_NONE;
    Value pst[2];
    Value material[2];
    uint64_t kingBlockers[2];
    uint64_t checkers = 0;
    uint64_t hashKey = 0;
    uint64_t pawnKey = 0;
    uint64_t materialKey = 0;

}StateInfo;

static const int MvvLvaVictim[5]   = { 100, 200, 300, 400, 500 };
static const int MvvLvaAttacker[6] = { 1, 2, 3, 4, 5, 0 };

// Board class: Stores all necessary information e.g bitboards and piecelists
class Board {

    public:

        std::vector<Move> moves;

        inline Color turn() const { return stm; }

        inline uint64_t pieces(const Color color) const { return bbColors[color]; }
        inline uint64_t pieces(const Piecetype pt) const { return bbPieces[pt]; }
        inline uint64_t pieces(const Color color, const Piecetype pt) const { return bbColors[color] & bbPieces[pt]; }

        inline Color owner(const unsigned sq) const { return Color(!(bbColors[WHITE] & SQUARES[sq])); }
        inline Piecetype piecetype(const unsigned sq) const { return pieceTypes[sq]; }

        inline uint64_t checkers() const { return state.checkers; }
        inline unsigned castleRights() const { return state.castling; }

        inline unsigned enPassant() const { return state.enPassant; }

        inline uint64_t hashkey() const { return state.hashKey; }
        inline uint64_t materialkey() const { return state.materialKey; }
        inline uint64_t pawnkey() const { return state.pawnKey; }

        inline unsigned plies() const { return ply; }
        inline void reset_plies() { ply = 0; }

        inline Value material(const Color color) const { return state.material[color]; }
        inline Value pst     (const Color color) const { return state.pst[color];      }

        inline unsigned piececount(const Color color, const Piecetype pt) const { return pieceCounts[color][pt]; }
        inline unsigned scale() const;

        void set_fen(std::string fen);
        void print() const;

        bool do_move(const Move move);
        void undo_move();
        void do_nullmove();
        void undo_nullmove();

        bool checkDraw();
        bool is_material_draw() const;

        bool is_valid(const Move move) const;
        bool is_legal(const Move move) const;
        bool is_castling_valid(const unsigned flag) const;
        inline bool can_castle(const unsigned flag) const;
        bool gives_check(const Move move);

        int mvvlva(const Move move) const;
        int see(const Move move) const;

        inline uint64_t minors_or_majors(const Color color) const;
        inline uint64_t majors() const;
        inline uint64_t sliders(const Color color) const;

        inline bool is_capture(const Move move) const;
        inline uint64_t getAllOccupancy(const uint64_t squares) const;
        inline uint64_t getPieceOccupancy(const uint64_t squares, const Piecetype type);
        uint64_t get_king_blockers(const Color color) const;
        uint64_t get_slider_blockers(const uint64_t sliders, const unsigned sq) const;

        inline uint64_t gen_wpawns_attacks() const;
        inline uint64_t gen_bpawns_attacks() const;
        inline uint64_t gen_pawns_attacks(const Color color) const;
        inline uint64_t get_same_colored_squares(const unsigned sq) const;

    private:

        StateInfo state;

        std::vector<StateInfo> states;

        // Bitboards
        uint64_t bbColors[3];
        uint64_t bbPieces[6];

        // Piecetypes
        Piecetype pieceTypes[64];

        // Piece type counts
        unsigned pieceCounts[2][6];

        // Color to move
        Color stm;

        // Plies
        unsigned int ply = 0;

        void clear();

        inline void hash_pawn(const Color color, const unsigned sq);
        inline void hash_piece(const Color color, const Piecetype pt, const unsigned sq);
        inline void hash_castling();
        inline void hash_turn();
        inline void hash_enPassant();
        inline void hash_material(const Color color, const Piecetype pt);

        void calc_keys();

        void add_piece(const Color color, const Piecetype pt, const unsigned sq);
        void remove_piece(const unsigned sq);
        void move_piece(const unsigned fromSq, const unsigned toSq);

        inline uint64_t pseudo_bb(const Piecetype pt, const Color color, const unsigned sq) const;

        inline bool singleSquareAttacked(uint64_t squares, const Color color) const;

        inline uint64_t sq_attackers(const Color color, const unsigned sq, const uint64_t occupied) const;
        inline uint64_t slider_attackers(const unsigned sq, const uint64_t occupied) const;
        inline uint64_t slider_attackers(const unsigned sq, const uint64_t occupied, const Color color) const;
        inline uint64_t slider_attackers_discovered(const Color color, const unsigned sq, const unsigned fromSq, const unsigned toSq) const;
        inline bool sq_attacked(const unsigned sq, const Color color) const;
        inline bool sq_attacked_noking(const unsigned sq, const Color color) const;

        uint64_t least_valuable_piece(uint64_t attackers, const Color color, Piecetype& pt) const;

        void update_check_info();

};

inline unsigned Board::scale() const {

    return std::max(((24 - 4 * int(pieceCounts[WHITE][QUEEN] + pieceCounts[BLACK][QUEEN]) - 2 * int(pieceCounts[WHITE][ROOK] + pieceCounts[BLACK][QUEEN]) - int(pieceCounts[WHITE][BISHOP] + pieceCounts[BLACK][BISHOP]) - int(pieceCounts[WHITE][KNIGHT] + pieceCounts[BLACK][KNIGHT])) * 256 + 12) / 24, 0);

}

inline bool Board::can_castle(const unsigned flag) const {

    return state.castling & flag;

}

inline uint64_t Board::minors_or_majors(const Color color) const {

    return (bbPieces[KNIGHT] | bbPieces[BISHOP] | bbPieces[ROOK] | bbPieces[QUEEN]) & bbColors[color];

}

inline uint64_t Board::majors() const {

    return bbPieces[ROOK] | bbPieces[QUEEN];

}

inline uint64_t Board::sliders(const Color color) const {

    return (bbPieces[BISHOP] | bbPieces[ROOK] | bbPieces[QUEEN]) && bbColors[color];

}

inline bool Board::is_capture(const Move move) const {

    return (pieceTypes[to_sq(move)] != PIECE_NONE || move_type(move) == ENPASSANT);

}

inline uint64_t Board::getAllOccupancy(const uint64_t squares) const {

    return bbColors[BOTH] & squares;

}

inline uint64_t Board::getPieceOccupancy(const uint64_t squares, const Piecetype type) {

    return bbPieces[type] & squares;

}

inline uint64_t Board::get_king_blockers(const Color color) const {

    return state.kingBlockers[color];

}

inline uint64_t Board::get_same_colored_squares(const unsigned sq) const {

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
inline void Board::hash_pawn(const Color color, const unsigned sq) {

    state.pawnKey ^= PawnHashKeys[color][sq];

}

// Hash piece in/out zobrist key
inline void Board::hash_piece(const Color color, const Piecetype pt, const unsigned sq) {

    state.hashKey ^= PieceHashKeys[color][pt][sq];

}

// Hash castling in/out zobrist key
inline void Board::hash_castling() {

    state.hashKey ^= CastlingHashKeys[state.castling];

}

// Hash turn in/out zobrist key
inline void Board::hash_turn() {

    state.hashKey ^= TurnHashKeys[stm];

}

// Hash enPassant in/out zobrist key
inline void Board::hash_enPassant() {

    state.hashKey ^= (state.enPassant != SQUARE_NONE) ? EnPassantHashKeys[file(state.enPassant)] : 0;

}

// Hash material
inline void Board::hash_material(const Color color, const Piecetype pt) {

    state.materialKey ^= MaterialHashKeys[color][pt][pieceCounts[color][pt]];

}

// Get all attackers to a square by a color
inline uint64_t Board::sq_attackers(const Color color, const unsigned sq, const uint64_t occupied) const {

    return  ((PawnAttacks[!color][sq]           & bbPieces[PAWN])
           | (KnightAttacks[sq]                 & bbPieces[KNIGHT])
           | (gen_bishop_moves(sq, occupied, 0) & (bbPieces[BISHOP] | bbPieces[QUEEN]))
           | (gen_rook_moves(sq, occupied, 0)   & (bbPieces[ROOK]   | bbPieces[QUEEN]))
           | (KingAttacks[sq]                   & bbPieces[KING])) & bbColors[color];

}

// Get all sliding attackers to a given square
inline uint64_t Board::slider_attackers(const unsigned sq, const uint64_t occupied) const {

    return (gen_bishop_moves(sq, occupied, 0) & (bbPieces[BISHOP] | bbPieces[QUEEN]))
         | (gen_rook_moves(sq, occupied, 0)   & (bbPieces[ROOK]   | bbPieces[QUEEN]));

}

inline uint64_t Board::slider_attackers(const unsigned sq, const uint64_t occupied, const Color color) const {

    return slider_attackers(sq, occupied) & bbColors[color];
}

inline uint64_t Board::slider_attackers_discovered(const Color color, const unsigned sq, const unsigned fromSq, const unsigned toSq) const {

    return slider_attackers(sq, (bbColors[BOTH] ^ SQUARES[fromSq]) | SQUARES[toSq], color) & ~SQUARES[fromSq];

}

// Check if a square is attacked by a given color
inline bool Board::sq_attacked(const unsigned sq, const Color color) const {

    return sq_attackers(color, sq, bbColors[BOTH]);

}

// Check if a square is attacked by a given color without the enemy king on the board
inline bool Board::sq_attacked_noking(const unsigned sq, const Color color) const {

    return sq_attackers(color, sq, bbColors[BOTH] ^ pieces(!color, KING));

}

inline uint64_t Board::gen_wpawns_attacks() const {

    return ((pieces(WHITE, PAWN) & ~FILE_A) << 9) | ((pieces(WHITE, PAWN) & ~FILE_H) << 7);

}

inline uint64_t Board::gen_bpawns_attacks() const {

    return ((pieces(BLACK, PAWN) & ~FILE_A) >> 7) | ((pieces(BLACK, PAWN) & ~FILE_H) >> 9);

}

inline uint64_t Board::gen_pawns_attacks(const Color color) const {

    return (color == WHITE) ? gen_wpawns_attacks() : gen_bpawns_attacks();

}

// Get a bitboards with all pseudo-legal destination squares for a slider on the given square
inline uint64_t slider_pseudo_bb(const Piecetype pt, const unsigned sq, const uint64_t occupied, const uint64_t friendly) {

    return (pt == BISHOP) ? gen_bishop_moves(sq, occupied, friendly) : (pt == ROOK) ? gen_rook_moves(sq, occupied, friendly) : get_queen_moves(sq, occupied, friendly);

}

// Get a bitboard with all pseudo-legal destination squares for a piece on the given square
inline uint64_t Board::pseudo_bb(const Piecetype pt, const Color color, const unsigned int sq) const {

    return ((pt == BISHOP || pt == ROOK || pt == QUEEN) ? slider_pseudo_bb(pt, sq, bbColors[BOTH], bbColors[color])
                                                       : pt == KNIGHT ? KnightAttacks[sq]
                                                       : pt == PAWN   ? PawnAttacks[color][sq]
                                                       : KingAttacks[sq]) & ~bbColors[color];

}

#endif
