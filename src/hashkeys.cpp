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

#include "hashkeys.hpp"

uint64_t pieceHashKeys[16][64];
uint64_t pawnHashKeys[2][64];
uint64_t materialHashKeys[14][64];
uint64_t turnHashKeys[2];
uint64_t castlingHashKeys[16];
uint64_t enPassantHashKeys[9];

void init_hashkeys() {

    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 64; j++) {
            pieceHashKeys[i][j] = rand64();
        }
        castlingHashKeys[i] = rand64();
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 64; j++) {
            pawnHashKeys[i][j] = rand64();
        }
    }

    for (int i = 0; i < 14; i++) {
        for (int j = 0; j < 64; j++) {
            materialHashKeys[i][j] = rand64();
        }
    }

    for (int i = 0; i < 9; i++) {
        enPassantHashKeys[i] = rand64();
    }

    turnHashKeys[0] = rand64();
    turnHashKeys[1] = rand64();


}

// Set hash table to size(in MB)
void TranspositionTable::setSize(const int hashsize) {

    //delete[] table;
    size = (MB * hashsize) / sizeof(TTEntry);
    table = new TTEntry [size];

}

void MaterialTable::clear() {

    MaterialEntry * entry;

    for (entry = table; entry < table + size; entry++) {
        entry->key = 0;
        entry->score = S( 0, 0 );
    }

}

MaterialEntry * MaterialTable::probe(const uint64_t key) {

    MaterialEntry * entry = &(table[key % size]);

    if (entry->key == key) {
        return entry;
    }

    return NULL;

}

void MaterialTable::store(const uint64_t key, const Score score) {

    table[key % size] = { key, score };

}

void PawnTable::clear() {

    PawnEntry * entry;

    for (entry = table; entry < table + size; entry++) {
        entry->key = 0;
        entry->score = S( 0, 0 );
        entry->weakPawns = 0;
        entry->passedPawns = 0;
        entry->pawnWAttacksSpan = 0;
        entry->pawnBAttacksSpan = 0;
    }

}

PawnEntry * PawnTable::probe(const uint64_t key) {

    PawnEntry * entry = &(table[key % size]);

    if (entry->key == key) {
        return entry;
    }

    return NULL;

}

void PawnTable::store(const uint64_t key, const Score score, const uint64_t pawnWAttacks, const uint64_t pawnBAttacks, const uint64_t weakPawns, const uint64_t passedPawns, const uint64_t pawnWAttacksSpan, const uint64_t pawnBAttacksSpan) {

    table[key % size] = { key, score, pawnWAttacks, pawnBAttacks, weakPawns, passedPawns, pawnWAttacksSpan, pawnBAttacksSpan };

}

void TranspositionTable::clear() {

    TTEntry * entry;

    for (entry = table; entry < table + size; entry++) {
        entry->key = 0;
        entry->depth = DEPTH_NONE;
        entry->flag = BOUND_NONE;
        entry->value = VALUE_NONE;
        entry->eval = VALUE_NONE;
        entry->bestMove = MOVE_NONE;
    }

}

// Check if hashkey in table, and if so, return the saved score for the position
TTEntry * TranspositionTable::probe(const uint64_t key, bool& tthit) {

    TTEntry * entry = &(table[key % size]);

    if (entry->key == key) {
        tthit = true;
        return entry;
    }

    tthit = false;
    return NULL;

}

// Get a hashentry from zobrist key, depth, value and bestMove and save it into the hashtable
void TranspositionTable::store(const uint64_t key, const int depth, const int value, const int eval, const Move bestMove, const int flag) {

    table[key % size] = { key, depth, flag, value, eval, bestMove };

}
