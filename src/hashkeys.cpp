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

#include "hashkeys.hpp"

uint64_t PieceHashKeys[2][7][64];
uint64_t PawnHashKeys[2][64];
uint64_t MaterialHashKeys[2][6][9];
uint64_t TurnHashKeys[2];
uint64_t CastlingHashKeys[16];
uint64_t EnPassantHashKeys[8];

void init_hashkeys() {

    for (unsigned c = WHITE; c < BOTH; c++) {
        for (unsigned sq = 0; sq < 64; sq++) {
            for (unsigned pt = PAWN; pt < PIECE_NONE+1; pt++) {
                PieceHashKeys[c][pt][sq] = rand64();
            }
            PawnHashKeys[c][sq] = rand64();
        }
        for (unsigned pt = PAWN; pt < PIECE_NONE; pt++) {
            for (unsigned i = 0; i < 10; i++) {
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
        entry->value = V( 0, 0 );
    }

}

MaterialEntry * MaterialTable::probe(const uint64_t key) {

    MaterialEntry * entry = &(table[key % size]);

    if (entry->key == key) {
        return entry;
    }

    return NULL;

}

void MaterialTable::store(const uint64_t key, const Value value) {

    table[key % size] = { key, value };

}

void PawnTable::clear() {

    PawnEntry * entry;

    for (entry = table; entry < table + size; entry++) {
        entry->key = 0;
        entry->value = V( 0, 0 );
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

void PawnTable::store(const uint64_t key, const Value value, const uint64_t pawnWAttacks, const uint64_t pawnBAttacks, const uint64_t passedPawns, const uint64_t pawnWAttacksSpan, const uint64_t pawnBAttacksSpan) {

    table[key % size] = { key, value, pawnWAttacks, pawnBAttacks, passedPawns, pawnWAttacksSpan, pawnBAttacksSpan };

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

// Check if hashkey in table, and if so, return the saved value for the position
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
