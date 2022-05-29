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

#ifndef BOARD_H
#define BOARD_H

#include "types.hpp"
#include "move.hpp"
#include "hashkeys.hpp"
#include "movegen.hpp"
#include "bitboards.hpp"

// FEN string of the inital position in chess
static const std::string INITIAL_POSITION_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

struct StateInfo {

    CastleRight castlingRights = 0u; // Castling rights
    Square enPassant = SQUARE_NONE; // En-passant square
    unsigned fiftyMoves = 0; // Fifty moves counter
    unsigned repetitionCount = 0;
    Piecetype captured = PIECE_NONE; // Last captured piece, necessary for undoing a move
    EvalTerm pst[2]; // Piece Square Table balances
    EvalTerm material[2]; // Material balances
    Bitboard kingBlockers[2]; // Pieces blocking sliding attacks to the kings
    Bitboard checkers = 0; // Pieces attacking the king
    uint64_t hashKey = 0;
    uint64_t pawnKey = 0;
    uint64_t materialKey = 0;

    inline void set_castling_right(CastleRight right) {
        castlingRights |= right;
    }

};

static const int MvvLvaVictim[5]   = { 100, 200, 300, 400, 500 };
static const int MvvLvaAttacker[6] = { 1, 2, 3, 4, 5, 0 };

// Board class
// This object stores all necessary information for representing a chess position:
//
//   - Bitboards for each type of piece and two bitboards identifying the owner of those pieces
//   - A list of all played moves on the board
//   - A state object which holds all the hash keys and other informations like fifty moves counter, en-passant square...
//   - Various functions for obtaining bitboards of a given piece type or a color
//   - Functions for moving pieces
//   - Functions for modifying the hash keys in the state object

class Board {

    public:

        inline Color turn() const { return stm; }

        inline Bitboard pieces(const Color color) const { return bbColors[color]; }
        inline Bitboard pieces(const Piecetype pt) const { return bbPieces[pt]; }
        inline Bitboard pieces(const Color color, const Piecetype pt) const { return bbColors[color] & bbPieces[pt]; }

        inline Bitboard empty_squares() const { return ~bbColors[BOTH]; }

        inline Color owner(const Square sq) const { return Color(!(bbColors[WHITE] & SQUARES[sq])); }
        inline Piecetype piecetype(const Square sq) const { return pieceTypes[sq]; }
        inline bool is_sq_empty(const Square sq) const { return pieceTypes[sq] == PIECE_NONE; }

        inline Bitboard checkers() const { return state.checkers; }
        inline CastleRight castleRights() const { return state.castlingRights; }

        inline Square enpassant_square() const { return state.enPassant; }
        inline Square king_square(const Color color) const { return lsb_index(bbPieces[KING] & bbColors[color]); };

        inline uint64_t hashkey() const { return state.hashKey; }
        inline uint64_t materialkey() const { return state.materialKey; }
        inline uint64_t pawnkey() const { return state.pawnKey; }

        inline unsigned plies() const { return ply; }
        inline void reset_plies() { ply = 0; }

        inline EvalTerm material(const Color color) const { return state.material[color]; }
        inline EvalTerm pst     (const Color color) const { return state.pst[color];      }

        inline unsigned piececount(const Color color, const Piecetype pt) const { return pieceCounts[color][pt]; }
        inline unsigned scale() const;

        void set_fen(std::string fen);
        std::string get_fen() const;
        std::string to_string() const;
        void print() const { std::cout << to_string(); }

        void do_move(const Move move);
        void undo_move();
        void do_nullmove();
        void undo_nullmove();

        bool check_draw();
        bool is_material_draw() const;

        bool is_valid(const Move move) const;
        bool is_legal(const Move move) const;
        bool is_castling_valid(const unsigned type) const;
        inline bool may_castle(const CastleRight right) const;
        bool gives_check(const Move move);

        int mvvlva(const Move move) const;
        int see(const Move move) const;

        inline Bitboard minors_and_majors(const Color color) const;
        inline Bitboard majors() const;
        inline Bitboard sliders() const;
        inline Bitboard sliders(const Color color) const;

        inline bool is_capture(const Move move) const;
        inline bool is_dangerous_pawn_push(const Move move) const;
        Bitboard get_king_blockers(const Color color) const;
        Bitboard get_slider_blockers(const uint64_t sliders, const Square sq) const;

        inline Bitboard gen_white_pawns_attacks() const;
        inline Bitboard gen_black_pawns_attacks() const;
        inline Bitboard gen_pawns_attacks(const Color color) const;
        inline Bitboard get_same_colored_squares(const Square sq) const;

    private:

        // Current board state
        StateInfo state;

        // List of all played moves on the board
        std::vector<Move> moves;

        // List of previous board states
        std::vector<StateInfo> states;

        // Bitboards for each piece type and each color
        std::array<Bitboard, COLOR_COUNT+1> bbColors;
        std::array<Bitboard, PIECETYPE_COUNT> bbPieces;

        // Piecetypes
        std::array<Piecetype, SQUARE_COUNT> pieceTypes;

        // Piece type counts
        std::array<std::array<unsigned, 6>,2> pieceCounts;

        // Color to move
        Color stm;

        // Number of half-moves played on the board so far
        unsigned ply = 0;

        void clear();

        inline void hash_pawn(const Color color, const Square sq);
        inline void hash_piece(const Color color, const Piecetype pt, const Square sq);
        inline void hash_castling();
        inline void hash_turn();
        inline void hash_enPassant();
        inline void hash_material(const Color color, const Piecetype pt);

        void calc_keys();

        void add_piece(const Color color, const Piecetype pt, const Square sq);
        void remove_piece(const Square sq);
        void move_piece(const Square fromSq, const Square toSq);

        inline Bitboard piece_attacks(const Piecetype pt, const Color color, const Square sq) const;

        inline Bitboard sq_attackers(const Color color, const Square sq, const Bitboard occupied) const;
        inline Bitboard slider_attackers(const Square sq, const Bitboard occupied) const;
        inline Bitboard slider_attackers(const Square sq, const Bitboard occupied, const Color color) const;
        inline Bitboard slider_attackers_discovered(const Color color, const Square sq, const Square fromSq, const Square toSq) const;
        inline bool sq_attacked(const Square sq, const Color color) const;
        inline bool sq_attacked_noking(const Square sq, const Color color) const;

        unsigned least_valuable_piece(Bitboard attackers, const Color color) const;

        void update_check_info();
        void update_repetition_count();

};

// Computes the scale factor for evaluation (taken from Fruit chess engine by Fabien Letouzy)
// The scale factor determines wether the midgame or endgame term should have more weight
// If there are less pieces left on the board, this means we are usually in an endgame
inline unsigned Board::scale() const {

    return std::max(((24 - 4 * int(pieceCounts[WHITE][QUEEN] + pieceCounts[BLACK][QUEEN]) - 2 * int(pieceCounts[WHITE][ROOK] + pieceCounts[BLACK][QUEEN]) - int(pieceCounts[WHITE][BISHOP] + pieceCounts[BLACK][BISHOP]) - int(pieceCounts[WHITE][KNIGHT] + pieceCounts[BLACK][KNIGHT])) * 256 + 12) / 24, 0);

}

// Returns true if the given castling move can still be performed
inline bool Board::may_castle(const CastleRight right) const {

    return state.castlingRights & right;

}

// Returns a bitboard of all minor and major pieces for a given color, essentially all pieces except pawns and the king
inline Bitboard Board::minors_and_majors(const Color color) const {

    return (bbPieces[KNIGHT] | bbPieces[BISHOP] | bbPieces[ROOK] | bbPieces[QUEEN]) & bbColors[color];

}

// Returns a bitboard of all major pieces for both colors
inline Bitboard Board::majors() const {

    return bbPieces[ROOK] | bbPieces[QUEEN];

}

// Returns a bitboard of all sliding pieces
inline Bitboard Board::sliders() const {

    return (bbPieces[BISHOP] | bbPieces[ROOK] | bbPieces[QUEEN]);

}

// Returns a bitboard of all sliding pieces for a given color
inline Bitboard Board::sliders(const Color color) const {

    return sliders() & bbColors[color];

}

// Checks wether a given move captures a piece in for the current position
inline bool Board::is_capture(const Move move) const {

    return (pieceTypes[to_sq(move)] != PIECE_NONE || move_type(move) == ENPASSANT);

}

// Check wether the given move is a dangerous pawn push for the current position
inline bool Board::is_dangerous_pawn_push(const Move move) const {

    return pieceTypes[from_sq(move)] == PAWN && relative_rank(stm, to_sq(move)) > RANK_5;

}

// Returns a bitboard of all pieces blocking the attacks of all enemy sliding pieces to the own king
inline Bitboard Board::get_king_blockers(const Color color) const {

    return state.kingBlockers[color];

}

// Returns a bitboard of all squares of the same color of the given square
inline Bitboard Board::get_same_colored_squares(const Square sq) const {

    return (SQUARES[sq] & SQUARES_WHITE) ? SQUARES_WHITE : SQUARES_BLACK;

}

// Hash pawn key in/out of key
inline void Board::hash_pawn(const Color color, const Square sq) {

    state.pawnKey ^= PawnHashKeys[color][sq];

}

// Hash piece key in/out zobrist key
inline void Board::hash_piece(const Color color, const Piecetype pt, const Square sq) {

    state.hashKey ^= PieceHashKeys[color][pt][sq];

}

// Hash castling key in/out zobrist key
inline void Board::hash_castling() {

    state.hashKey ^= CastlingHashKeys[state.castlingRights];

}

// Hash turn key in/out zobrist key
inline void Board::hash_turn() {

    state.hashKey ^= TurnHashKeys[stm];

}

// Hash enPassant key in/out zobrist key
inline void Board::hash_enPassant() {

    state.hashKey ^= (state.enPassant != SQUARE_NONE) ? EnPassantHashKeys[file(state.enPassant)] : 0;

}

// Hash material key in/out zobrist key
inline void Board::hash_material(const Color color, const Piecetype pt) {

    state.materialKey ^= MaterialHashKeys[color][pt][pieceCounts[color][pt]];

}

// Get a bitboard of all attackers of a color to a square
inline Bitboard Board::sq_attackers(const Color color, const Square sq, const Bitboard occupied) const {

    return  ((PawnAttacks[!color][sq]           & bbPieces[PAWN])
           | (KnightAttacks[sq]                 & bbPieces[KNIGHT])
           | (bishop_target_squares(sq, occupied, 0) & (bbPieces[BISHOP] | bbPieces[QUEEN]))
           | (rook_target_squares(sq, occupied, 0)   & (bbPieces[ROOK]   | bbPieces[QUEEN]))
           | (KingAttacks[sq]                   & bbPieces[KING])) & bbColors[color];

}

// Get a bitboard of all sliding attackers to a square
inline Bitboard Board::slider_attackers(const Square sq, const Bitboard occupied) const {

    return (bishop_target_squares(sq, occupied, 0) & (bbPieces[BISHOP] | bbPieces[QUEEN]))
         | (rook_target_squares(sq, occupied, 0)   & (bbPieces[ROOK]   | bbPieces[QUEEN]));

}

// Get a bitboard of all sliding attackers of a color to a square
inline Bitboard Board::slider_attackers(const Square sq, const Bitboard occupied, const Color color) const {

    return slider_attackers(sq, occupied) & bbColors[color];
}

// Get a bitboard of all sliding attackers if we move a piece on the board
inline Bitboard Board::slider_attackers_discovered(const Color color, const Square sq, const Square fromSq, const Square toSq) const {

    return slider_attackers(sq, (bbColors[BOTH] ^ SQUARES[fromSq]) | SQUARES[toSq], color) & ~SQUARES[fromSq];

}

// Check if a square is attacked by a given color
inline bool Board::sq_attacked(const Square sq, const Color color) const {

    return sq_attackers(color, sq, bbColors[BOTH]);

}

// Check if a square is attacked by a given color without the opponent's king on the board
inline bool Board::sq_attacked_noking(const Square sq, const Color color) const {

    return sq_attackers(color, sq, bbColors[BOTH] ^ pieces(!color, KING));

}

// Get a bitboard of all attacks by all white pawns on the given board
inline uint64_t Board::gen_white_pawns_attacks() const {

    return ((pieces(WHITE, PAWN) & ~BB_FILE_A) << 9) | ((pieces(WHITE, PAWN) & ~BB_FILE_H) << 7);

}

// Get a bitboard of all attacks by all black pawns on the given board
inline Bitboard Board::gen_black_pawns_attacks() const {

    return ((pieces(BLACK, PAWN) & ~BB_FILE_A) >> 7) | ((pieces(BLACK, PAWN) & ~BB_FILE_H) >> 9);

}

// Get a bitboard of all attacks by the pawns of the given color
inline Bitboard Board::gen_pawns_attacks(const Color color) const {

    return (color == WHITE) ? gen_white_pawns_attacks() : gen_black_pawns_attacks();

}

// Get a bitboards with all pseudo-legal destination squares for a slider on the given square
inline Bitboard slider_moves(const Piecetype pt, const Square sq, const Bitboard occupied, const Bitboard friendly) {

    return   (pt == BISHOP) ? bishop_target_squares(sq, occupied, friendly)
           : (pt == ROOK)   ? rook_target_squares(sq, occupied, friendly)
           : queen_target_squares(sq, occupied, friendly);

}

// Get a bitboard with all pseudo-legal attack squares for a piece on the given square
inline Bitboard Board::piece_attacks(const Piecetype pt, const Color color, const Square sq) const {

    return ((pt == BISHOP || pt == ROOK || pt == QUEEN) ? slider_moves(pt, sq, bbColors[BOTH], bbColors[color])
                                                        : pt == KNIGHT ? KnightAttacks[sq]
                                                        : pt == PAWN   ? PawnAttacks[color][sq]
                                                        : KingAttacks[sq]) & ~bbColors[color];

}

#endif
