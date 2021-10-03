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

#include <cstring>

#include "hashkeys.hpp"

// Hashkey arrays for generating a position hashkey
uint64_t PieceHashKeys[2][7][64];
uint64_t PawnHashKeys[2][64];
uint64_t MaterialHashKeys[2][6][11];
uint64_t TurnHashKeys[2];
uint64_t CastlingHashKeys[16];
uint64_t EnPassantHashKeys[8];

// Generate a random 64bit integer
// Source: http://vigna.di.unimi.it/ftp/papers/xorshift.pdf
static uint64_t x = 88172645463325252ULL;

static uint64_t rand64() {

    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x * 2685821657736338717LL;

}

// Initialize the hash keys
// For each color, we assign a hashkey to each square for each piece type
// Also, we assign hashkeys to a second array for each pawn of both colors for each square
// We also have hashkeys for all possible configurations of material on the board
// There are also hashkeys for all possible castling states and 8 hashkeys for each file where
// there could be a potential en-passant square.
// Furthermore, there are two additional hashkeys representing the current color to move
void init_hashkeys() {

    for (Color c = WHITE; c < BOTH; c++) {
        for (Square sq = 0; sq < 64; sq++) {
            for (Piecetype pt = PAWN; pt < PIECE_NONE+1; pt++) {
                PieceHashKeys[c][pt][sq] = rand64();
            }
            PawnHashKeys[c][sq] = rand64();
        }
        for (Piecetype pt = PAWN; pt < PIECE_NONE; pt++) {
            for (unsigned i = 0; i < 11; i++) {
                MaterialHashKeys[c][pt][i] = rand64();
            }
        }
    }

    for (unsigned i = 0; i < 16; i++) {
        CastlingHashKeys[i] = rand64();
    }

    for (unsigned i = 0; i < 8; i++) {
        EnPassantHashKeys[i] = rand64();
    }

    TurnHashKeys[WHITE] = rand64();
    TurnHashKeys[BLACK] = rand64();

}

// Set hash table to a given size in megabytes
void TranspositionTable::set_size(const unsigned megabytes) {

    // Free up memory
    delete[] table;

    bucketCount = MB * megabytes / sizeof(TTBucket);

    try {
        table = new TTBucket [bucketCount];
    } catch (std::bad_alloc& exception) {
        std::cerr << "Error: Failed to allocate memory of size "
                  << megabytes
                  << " megabytes for Transposition Table." << std::endl
                  << "OS message: " << exception.what();
        std::exit(EXIT_FAILURE);
    }

    clear();

}

// Clears the transposition hash table
void TranspositionTable::clear() {

    std::memset(table, 0, sizeof(TTBucket) * bucketCount);
    generation = 0;

}

// Increase Transposition Table age
void TranspositionTable::new_search() {

    generation += TT_MASK_BOUND + 1;

}

// Check if hashkey in table, and if so, return the saved value for the position
TTEntry * TranspositionTable::probe(const uint64_t key, bool& ttHit) {

    // Convert 64bit hash key to 16bit key
    const uint16_t key16 = key >> 48;
    TTEntry *entries     = table[key & TT_MASK_KEY].entries;

    for (unsigned entryIndex = 0; entryIndex < TT_BUCKET_SIZE; entryIndex++) {
        TTEntry *entry = &entries[entryIndex];
        if (entry->key() == key16) {
            // Update entry age
            entry->update_generation(generation);

            assert(entry->generation() == generation);

            ttHit = true;
            return entry;
        }
    }

    ttHit = false;
    return NULL;

}

// Get a hashentry from zobrist key, depth, value and bestMove and save it into the hashtable
void TranspositionTable::store(const uint64_t key, const Depth depth, const Value value, const Value eval, const Move bestMove, const TTBound bound) {

    const uint16_t key16 = key >> 48;
    TTEntry *entries     = table[key & TT_MASK_KEY].entries;
    TTEntry *replace     = &entries[0];

    for (unsigned entryIndex = 0; entryIndex < TT_BUCKET_SIZE; entryIndex++) {
        TTEntry *entry = &entries[entryIndex];
        // Prefer a matching hash key
        // Otherwise, replace entries with the lowest depth and highest age difference
        if (entry->key() == key16) {
            replace = entry;
            break;
        } else if (replace->depth() - (generation - replace->generation()) >=
                   entry->depth()   - (generation - entry->generation())) {
            replace = entry;
        }
    }

    // For entries with matching hash key, only overwrite for exact bounds
    // and if the depth is somewhat higher
    if (   bound != BOUND_EXACT
        && key16 == replace->key()
        && depth < replace->depth() - 3) {
        return;
    }

    // Otherwise, it is safe to store the entry
    replace->replace(key16, generation, bound, depth, value, eval, bestMove);
    
    assert(key16 == replace->key());
    assert(generation == replace->generation());
    assert(bound == replace->bound());
    assert(depth == replace->depth());
    assert(value == replace->value());
    assert(eval == replace->eval());
    assert(bestMove == replace->move());

}

// Return the number of used entries permill (for the first 1000 entries)
unsigned TranspositionTable::hashfull() {

    unsigned usedCount = 0;

    for (unsigned bucketIndex = 0; bucketIndex < 1000; bucketIndex++) {
        for (unsigned entryIndex = 0; entryIndex < TT_BUCKET_SIZE; entryIndex++) {
            usedCount +=    table[bucketIndex].entries[entryIndex].bound() != BOUND_NONE
                         && table[bucketIndex].entries[entryIndex].generation() == generation;
        }
    }

    return usedCount / TT_BUCKET_SIZE;

}

// Clears the material hash table
void MaterialTable::clear() {

    std::memset(table, size, sizeof(MaterialEntry) * size);

}

// Probes the material hash table and returns the corresponding entry
MaterialEntry * MaterialTable::probe(const uint64_t key) {

    MaterialEntry * entry = &(table[key % size]);

    if (entry->key == key) {
        return entry;
    }

    return NULL;

}

// Stores a material imbalance evaluation in the material hash table given the masterial hash key
void MaterialTable::store(const uint64_t key, const EvalTerm value) {

    table[key % size] = { key, value };

}

// Clears the pawn hash table
void PawnTable::clear() {

    std::memset(table, size, sizeof(PawnEntry) * size);

}

// Probes the pawn hash table and returns the corresponding entry
PawnEntry * PawnTable::probe(const uint64_t key) {

    PawnEntry * entry = &(table[key % size]);

    if (entry->key == key) {
        return entry;
    }

    return NULL;

}

// Stores a pawn structure evaluation value and several pawn related bitboards for a given pawn hash key
void PawnTable::store(const uint64_t key, const EvalTerm value, const uint64_t pawnWAttacks, const uint64_t pawnBAttacks, const uint64_t passedPawns, const uint64_t pawnWAttacksSpan, const uint64_t pawnBAttacksSpan) {

    table[key % size] = { key, value, pawnWAttacks, pawnBAttacks, passedPawns, pawnWAttacksSpan, pawnBAttacksSpan };

}