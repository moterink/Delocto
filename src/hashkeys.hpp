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

#ifndef HASHKEYS_H
#define HASHKEYS_H

#include <random>
#include "types.hpp"
#include "move.hpp"

// A megabyte
static constexpr unsigned MB = 0x100000;

// Transpositon Table Bucket Size
static constexpr unsigned TT_BUCKET_SIZE     = 3;
static constexpr uint64_t TT_MASK_KEY        = 0xFFFF;
static constexpr unsigned TT_MASK_BOUND      = 0x03;
static constexpr unsigned TT_MASK_GENERATION = 0xFC;
static constexpr unsigned TT_CYCLE           = std::numeric_limits<uint8_t>::max() + TT_MASK_BOUND + 1;

// Hash flags
enum TTBound : uint8_t {

    BOUND_NONE, BOUND_EXACT, BOUND_UPPER, BOUND_LOWER

};

// Hash entry
struct TTEntry {

    private: 
        uint16_t key16;
        uint8_t genBound8;
        int8_t depth8;
        int16_t value16;
        int16_t eval16;
        Move move16;

    public:
        uint16_t key()  const { return key16; }
        Move move()     const { return move16; }
        uint8_t generation() const { return genBound8 & TT_MASK_GENERATION; }
        TTBound bound() const { return (TTBound)(genBound8 & TT_MASK_BOUND); }
        Depth depth()   const { return (Depth)depth8; }
        Value value()   const { return (Value)value16; }
        Value eval()    const { return (Value)eval16; }

        void replace(uint16_t key, uint8_t generation, TTBound bound, Depth depth, Value value, Value eval, Move move) {
            key16     = key;
            genBound8 = generation | bound;
            depth8    = depth;
            value16   = value;
            eval16    = eval;
            move16    = move;
        }

        void update_generation(const uint8_t generation) { genBound8 = generation | bound(); }


};

struct PawnEntry {

    uint64_t key;
    EvalTerm value;
    Bitboard pawnWAttacks;
    Bitboard pawnBAttacks;
    Bitboard passedPawns;
    Bitboard pawnWAttacksSpan;
    Bitboard pawnBAttacksSpan;

};

struct MaterialEntry {

    uint64_t key;
    EvalTerm value;

};

// Transposition Table Class
class TranspositionTable {

    private:
        
        // Transposition Table Bucket
        struct TTBucket {
            TTEntry entries[TT_BUCKET_SIZE];
        };

        TTBucket *table;
        unsigned bucketCount;

        uint8_t generation; // Will start a new cycle at 0 when it exceeds the numeric limit

    public:

        void set_size(const unsigned megabytes);
        void clear();
        void new_search();
        TTEntry * probe(const uint64_t key, bool& ttHit);
        void store(const uint64_t key, const Depth depth, const Value value, const Value eval, const Move bestMove, const TTBound bound);
        unsigned hashfull();

        ~TranspositionTable() {
            delete[] table;
        }

};

class PawnTable {

    public:

        unsigned size = 0x10000;

        PawnEntry *table;

        void clear();
        PawnEntry * probe(const uint64_t key);
        void store(const uint64_t key, const EvalTerm value, const uint64_t pawnWAttacks, const uint64_t pawnBAttacks, const uint64_t passedPawns, const uint64_t pawnWAttacksSpan, const uint64_t pawnBAttacksSpan);

        PawnTable() {
            table = new PawnEntry [size];
            clear();
        }

        ~PawnTable() {
            delete[] table;
        }

};

class MaterialTable {

    public:

        unsigned size = 0x2000;

        MaterialEntry *table;

        void clear();
        MaterialEntry * probe(const uint64_t key);
        void store(const uint64_t key, const EvalTerm value);

        MaterialTable() {
            table = new MaterialEntry [size];
            clear();
        }

        ~MaterialTable() {
            delete[] table;
        }

};

// Convert a value for the transposition table, since if the value is a mate value,
// the plies until mate have to be consistent
inline int value_to_tt(Value value, Depth plies) {

    assert(value != VALUE_NONE);

    return   value >= VALUE_MATE_MAX  ? value + plies
           : value <= VALUE_MATED_MAX ? value - plies
           : value;

}

// Adjust a potential mate value from the transposition table for the current number
// of plies
inline int value_from_tt(Value value, Depth plies) {

    return value == VALUE_NONE ? VALUE_NONE 
                               : value >= VALUE_MATE_MAX  ? value - plies
                               : value <= VALUE_MATED_MAX ? value + plies
                               : value;

}

extern uint64_t PieceHashKeys[2][7][64];
extern uint64_t PawnHashKeys[2][64];
extern uint64_t MaterialHashKeys[2][6][11];
extern uint64_t TurnHashKeys[2];
extern uint64_t CastlingHashKeys[16];
extern uint64_t EnPassantHashKeys[8];

extern void init_hashkeys();

#endif
