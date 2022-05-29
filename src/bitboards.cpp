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

#include "bitboards.hpp"
#include "movegen.hpp"
#include "evaluate.hpp"

// Directions of movement for bishops and rooks
constexpr int BishopDirections[4] = { LEFTUP, LEFTDOWN, RIGHTUP, RIGHTDOWN };
constexpr int RookDirections[4]   = {   LEFT,    RIGHT,      UP,      DOWN };

// Magic numbers from Ethereal chess engine
constexpr uint64_t Magics[2][64] = {

    {
        0xFFEDF9FD7CFCFFFFull, 0xFC0962854A77F576ull, 0x5822022042000000ull, 0x2CA804A100200020ull, 0x0204042200000900ull, 0x2002121024000002ull, 0xFC0A66C64A7EF576ull, 0x7FFDFDFCBD79FFFFull,
        0xFC0846A64A34FFF6ull, 0xFC087A874A3CF7F6ull, 0x1001080204002100ull, 0x1810080489021800ull, 0x0062040420010A00ull, 0x5028043004300020ull, 0xFC0864AE59B4FF76ull, 0x3C0860AF4B35FF76ull,
        0x73C01AF56CF4CFFBull, 0x41A01CFAD64AAFFCull, 0x040C0422080A0598ull, 0x4228020082004050ull, 0x0200800400E00100ull, 0x020B001230021040ull, 0x7C0C028F5B34FF76ull, 0xFC0A028E5AB4DF76ull,
        0x0020208050A42180ull, 0x001004804B280200ull, 0x2048020024040010ull, 0x0102C04004010200ull, 0x020408204C002010ull, 0x02411100020080C1ull, 0x102A008084042100ull, 0x0941030000A09846ull,
        0x0244100800400200ull, 0x4000901010080696ull, 0x0000280404180020ull, 0x0800042008240100ull, 0x0220008400088020ull, 0x04020182000904C9ull, 0x0023010400020600ull, 0x0041040020110302ull,
        0xDCEFD9B54BFCC09Full, 0xF95FFA765AFD602Bull, 0x1401210240484800ull, 0x0022244208010080ull, 0x1105040104000210ull, 0x2040088800C40081ull, 0x43FF9A5CF4CA0C01ull, 0x4BFFCD8E7C587601ull,
        0xFC0FF2865334F576ull, 0xFC0BF6CE5924F576ull, 0x80000B0401040402ull, 0x0020004821880A00ull, 0x8200002022440100ull, 0x0009431801010068ull, 0xC3FFB7DC36CA8C89ull, 0xC3FF8A54F4CA2C89ull,
        0xFFFFFCFCFD79EDFFull, 0xFC0863FCCB147576ull, 0x040C000022013020ull, 0x2000104000420600ull, 0x0400000260142410ull, 0x0800633408100500ull, 0xFC087E8E4BB2F736ull, 0x43FF9E4EF4CA2C89ull,

    },
    {
        0xA180022080400230ull, 0x0040100040022000ull, 0x0080088020001002ull, 0x0080080280841000ull, 0x4200042010460008ull, 0x04800A0003040080ull, 0x0400110082041008ull, 0x008000A041000880ull,
        0x10138001A080C010ull, 0x0000804008200480ull, 0x00010011012000C0ull, 0x0022004128102200ull, 0x000200081201200Cull, 0x202A001048460004ull, 0x0081000100420004ull, 0x4000800380004500ull,
        0x0000208002904001ull, 0x0090004040026008ull, 0x0208808010002001ull, 0x2002020020704940ull, 0x8048010008110005ull, 0x6820808004002200ull, 0x0A80040008023011ull, 0x00B1460000811044ull,
        0x4204400080008EA0ull, 0xB002400180200184ull, 0x2020200080100380ull, 0x0010080080100080ull, 0x2204080080800400ull, 0x0000A40080360080ull, 0x02040604002810B1ull, 0x008C218600004104ull,
        0x8180004000402000ull, 0x488C402000401001ull, 0x4018A00080801004ull, 0x1230002105001008ull, 0x8904800800800400ull, 0x0042000C42003810ull, 0x008408110400B012ull, 0x0018086182000401ull,
        0x2240088020C28000ull, 0x001001201040C004ull, 0x0A02008010420020ull, 0x0010003009010060ull, 0x0004008008008014ull, 0x0080020004008080ull, 0x0282020001008080ull, 0x50000181204A0004ull,
        0x48FFFE99FECFAA00ull, 0x48FFFE99FECFAA00ull, 0x497FFFADFF9C2E00ull, 0x613FFFDDFFCE9200ull, 0xFFFFFFE9FFE7CE00ull, 0xFFFFFFF5FFF3E600ull, 0x0010301802830400ull, 0x510FFFF5F63C96A0ull,
        0xEBFFFFB9FF9FC526ull, 0x61FFFEDDFEEDAEAEull, 0x53BFFFEDFFDEB1A2ull, 0x127FFFB9FFDFB5F6ull, 0x411FFFDDFFDBF4D6ull, 0x0801000804000603ull, 0x0003FFEF27EEBE74ull, 0x7645FFFECBFEA79Eull,

    }

};

// Magic lookup tables for bishops and rooks
Magic BishopMagics[64];
Magic RookMagics[64];

uint64_t BishopMagicAttacks[0x1480];
uint64_t RookMagicAttacks[0x19000];

Bitboard PawnAttacksSpan[2][64]; // The adjacent files to a pawn for each color and every square
Bitboard KingShelterSpan[2][64]; // The three (or two on the edge) files ahead of the king
Bitboard KingRing[2][64]; // The squares surrounding the king. On the edge, an additional rank/file is added
Bitboard RayTable[64][64]; // The line intersecting two squares, excluding the start square
Bitboard LineTable[64][64]; // The line intersecting two squares

// Attack Bitboards
Bitboard PawnAttacks[2][64];
Bitboard KnightAttacks[64];
Bitboard BishopAttacks[64];
Bitboard RookAttacks[64];
Bitboard QueenAttacks[64];
Bitboard KingAttacks[64];

// Mask Bitboards
Bitboard FrontFileMask[2][64]; // Squares in front of each square until the board edge
Bitboard PassedPawnMask[2][64]; // The three (or two on the edge) files ahead of the square
Bitboard BackwardPawnMask[2][64]; // The adjacent files to a pawn with the squares left and right included

// Get a bitboard of all squares a slider can move to  from a given square
Bitboard get_slider_attacks(const Square sq, const Bitboard occupied, const int directions[4]) {

    Bitboard attacks = 0;

    // Move into every direction until we encounter a piece
    for (unsigned d = 0; d < 4; d++) {

        int nsq = sq;
        while (true) {
            int lsq = nsq;
            nsq += direction(WHITE, directions[d]);
            // Check if the square is still on the board and that we did not go beyond the board edge
            if (sq_valid(nsq) && KingDistance[lsq][nsq] == 1) {
                // Add the square to the bitboard. We do not care about the color of the piece, since we
                // will remove all friendly pieces anyway once we have the information
                attacks |= SQUARES[nsq];
                if (SQUARES[nsq] & occupied) {
                    break;
                }
            } else {
                break;
            }

        }

    }

    return attacks;

}

// Formula for getting the magic index for the given arrangement of pieces
int get_magic_index(const uint64_t occupied, Magic *table) {

    return ((occupied & table->mask) * table->magic) >> table->shift;

}

// Initialize the attack table for a given square so we can look up the pseudo-legal moves later
void init_magics(const Square sq, Magic *table, const uint64_t magic, const int directions[4]) {

    // Board edge
    const Bitboard edge = ((BB_RANK_1 | BB_RANK_8) & ~RANKS[rank(sq)]) | ((BB_FILE_A | BB_FILE_H) & ~FILES[file(sq)]);
    Bitboard occupied   = 0;

    table[sq].magic = magic; // Magic number
    table[sq].mask  = get_slider_attacks(sq, 0, directions) & ~edge; // All squares this piece can reach with no pieces on the board, excluding the edge
    table[sq].shift = 64 - popcount(table[sq].mask); // Magic shift

    if (sq != SQUARE_A8) {
        table[sq + 1].attacks = table[sq].attacks + (1 << popcount(table[sq].mask));
    }

    // Initialize the attack lookup table
    // For every possible configuration of pieces on the target squares,
    // there is a unique index generated by the get_magic_index() method
    // We then calculate the real moves considering the pieces which might
    // block our piece from moving
    do {
        int index = get_magic_index(occupied, &table[sq]);
        table[sq].attacks[index] = get_slider_attacks(sq, occupied, directions);
        occupied = (occupied - table[sq].mask) & table[sq].mask;
    } while(occupied);

}

// Initialize piece attack bitboards
void init_attacks() {

    // Loop over every square on the board
    for (Square sq = 0; sq < 64; sq++) {

        // Pawns only attack the two diagonal squares ahead of them
        PawnAttacks[WHITE][sq] = ((SQUARES[sq] & ~BB_FILE_A) << 9) | ((SQUARES[sq] & ~BB_FILE_H) << 7);
        PawnAttacks[BLACK][sq] = ((SQUARES[sq] & ~BB_FILE_A) >> 7) | ((SQUARES[sq] & ~BB_FILE_H) >> 9);

        KnightAttacks[sq] =   ((SQUARES[sq] & ~(BB_FILE_A | BB_RANK_8 | BB_RANK_7)) << 17)
                            | ((SQUARES[sq] & ~(BB_FILE_H | BB_RANK_8 | BB_RANK_7)) << 15)
                            | ((SQUARES[sq] & ~(BB_FILE_A | BB_FILE_B | BB_RANK_8)) << 10)
                            | ((SQUARES[sq] & ~(BB_FILE_H | BB_FILE_G | BB_RANK_8)) << 6)
                            | ((SQUARES[sq] & ~(BB_FILE_A | BB_FILE_B | BB_RANK_1)) >> 6)
                            | ((SQUARES[sq] & ~(BB_FILE_H | BB_FILE_G | BB_RANK_1)) >> 10)
                            | ((SQUARES[sq] & ~(BB_FILE_A | BB_RANK_1 | BB_RANK_2)) >> 15)
                            | ((SQUARES[sq] & ~(BB_FILE_H | BB_RANK_1 | BB_RANK_2)) >> 17);
        KingAttacks[sq]   =   ((SQUARES[sq] & ~(BB_FILE_A | BB_RANK_8)) << 9)
                            | ((SQUARES[sq] & ~BB_RANK_8) << 8)
                            | ((SQUARES[sq] & ~(BB_FILE_H | BB_RANK_8)) << 7)
                            | ((SQUARES[sq] & ~BB_FILE_A) << 1)
                            | ((SQUARES[sq] & ~BB_FILE_H) >> 1)
                            | ((SQUARES[sq] & ~(BB_FILE_A | BB_RANK_1)) >> 7)
                            | ((SQUARES[sq] & ~BB_RANK_1) >> 8)
                            | ((SQUARES[sq] & ~(BB_FILE_H | BB_RANK_1)) >> 9);

        BishopAttacks[sq] = get_slider_attacks(sq, 0, BishopDirections);
        RookAttacks[sq]   = get_slider_attacks(sq, 0, RookDirections);
        QueenAttacks[sq]  = BishopAttacks[sq] | RookAttacks[sq]; // Queen movement is bishop and rook movement combined

    }

}

// Initialize attack, magic and all mask bitboards
void init_bitboards() {

    // Initialize magic bitboards
    BishopMagics[0].attacks = BishopMagicAttacks;
    RookMagics[0].attacks   = RookMagicAttacks;

    for (Square sq = 0; sq < 64; sq++) {

        init_magics(sq, BishopMagics, Magics[0][sq], BishopDirections);
        init_magics(sq, RookMagics, Magics[1][sq], RookDirections);

    }

    // Initialize attack bitboards
    init_attacks();

    for (Square sq = 0; sq < 64; sq++) {

        Bitboard pawnsFrontW = 0, pawnsFrontB = 0, kingsFrontW = 0, kingsFrontB = 0;

        for (int i = 1; i < 6; i++) {
            pawnsFrontW |= SQUARES[sq] << (i * 8);
            pawnsFrontB |= SQUARES[sq] >> (i * 8);
            kingsFrontW |= pawnsFrontW;
            kingsFrontB |= pawnsFrontB;
        }

        PawnAttacksSpan[WHITE][sq] = ((pawnsFrontW & ~BB_FILE_A) << 1) | ((pawnsFrontW & ~BB_FILE_H) >> 1);
        PawnAttacksSpan[BLACK][sq] = ((pawnsFrontB & ~BB_FILE_A) << 1) | ((pawnsFrontB & ~BB_FILE_H) >> 1);
        KingShelterSpan[WHITE][sq] = ((kingsFrontW & ~BB_FILE_A) << 1) | ((kingsFrontW & ~BB_FILE_H) >> 1) | kingsFrontW;
        KingShelterSpan[BLACK][sq] = ((kingsFrontB & ~BB_FILE_A) << 1) | ((kingsFrontB & ~BB_FILE_H) >> 1) | kingsFrontB;

        KingRing[WHITE][sq] = KingAttacks[sq];
        KingRing[BLACK][sq] = KingAttacks[sq];
        if (relative_rank(WHITE, sq) == 0) {
            KingRing[WHITE][sq] |= shift_up(KingRing[WHITE][sq], WHITE);
        }
        if (relative_rank(BLACK, sq) == 0) {
            KingRing[BLACK][sq] |= shift_up(KingRing[BLACK][sq], BLACK);
        }
        if (file(sq) == 0) {
            KingRing[WHITE][sq] |= shift_left(KingRing[WHITE][sq], WHITE);
            KingRing[BLACK][sq] |= shift_right(KingRing[BLACK][sq], BLACK);
        }
        if (file(sq) == 7) {
            KingRing[WHITE][sq] |= shift_right(KingRing[WHITE][sq], WHITE);
            KingRing[BLACK][sq] |= shift_left(KingRing[BLACK][sq], BLACK);
        }

        for (unsigned i = 1; i < 8; i++) {
            Bitboard nsq = (SQUARES[sq] << (8 * i));
            FrontFileMask[WHITE][sq] |= nsq;
            if (nsq & BB_RANK_8)
                break;
        }

        for (unsigned i = 1; i < 8; i++) {
            Bitboard nsq = (SQUARES[sq] >> (8 * i));
            FrontFileMask[BLACK][sq] |= nsq;
            if (nsq & BB_RANK_1)
                break;
        }

    }

    for (Square sq1 = 0; sq1 < 64; sq1++) {

        const Bitboard bishopPseudoBB = BishopAttacks[sq1];
        const Bitboard rookPseudoBB   = RookAttacks[sq1];

        for (Square sq2 = 0; sq2 < 64; sq2++) {

            if (bishopPseudoBB & SQUARES[sq2]) {
                RayTable[sq1][sq2]  = (bishop_target_squares(sq1, SQUARES[sq2], SQUARES[sq2]) & bishop_target_squares(sq2, SQUARES[sq1], SQUARES[sq1])) | SQUARES[sq2];
                LineTable[sq1][sq2] = (bishop_target_squares(sq1, 0, 0) & bishop_target_squares(sq2, 0, 0)) | SQUARES[sq1] | SQUARES[sq2];
            } else if (rookPseudoBB & SQUARES[sq2]) {
                RayTable[sq1][sq2]  = (rook_target_squares(sq1, SQUARES[sq2], SQUARES[sq2]) & rook_target_squares(sq2, SQUARES[sq1], SQUARES[sq1])) | SQUARES[sq2];
                LineTable[sq1][sq2] = (rook_target_squares(sq1, 0, 0) & rook_target_squares(sq2, 0, 0)) | SQUARES[sq1] | SQUARES[sq2];
            }

        }

        int f = file(sq1);
        int r = rank(sq1);
        PassedPawnMask[WHITE][sq1] = FrontFileMask[WHITE][sq1] | (f != 0 ? FrontFileMask[WHITE][sq1-1] : 0) | (f != 7 ? FrontFileMask[WHITE][sq1+1] : 0);
        PassedPawnMask[BLACK][sq1] = FrontFileMask[BLACK][sq1] | (f != 0 ? FrontFileMask[BLACK][sq1-1] : 0) | (f != 7 ? FrontFileMask[BLACK][sq1+1] : 0);

        BackwardPawnMask[WHITE][sq1] = (r != 0 ? (f != 0 ? FrontFileMask[BLACK][sq1-9] : 0) | (f != 7 ? FrontFileMask[BLACK][sq1-7] : 0) : 0);
        BackwardPawnMask[BLACK][sq1] = (r != 7 ? (f != 0 ? FrontFileMask[WHITE][sq1+7] : 0) | (f != 7 ? FrontFileMask[WHITE][sq1+9] : 0) : 0);

    }

}

// Converts a bitboard to an ASCII-like representation
// with 1 representing occupied and 0 unoccupied squares
std::string bitboard_to_string(const Bitboard bitboard) {

    std::stringstream ss;

    for (unsigned rank = RANK_1; rank <= RANK_8; rank++) {
        for (unsigned file = FILE_H; file <= FILE_A; file++) {
            ss << (bitboard & SQUARES[rank * 8 + file] ? "1 " : "0 ");
        }
        ss << std::endl;
    }

    return ss.str();

}

// This is for debugging purposes only - when using debuggers, newlines are usually escaped
// Therefor the only way we can show this ascii representation of the bitboard is in stdout
void print_bitboard(const Bitboard bitboard) {

    std::cout << bitboard_to_string(bitboard) << std::endl;

}