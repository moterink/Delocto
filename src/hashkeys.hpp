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

#ifndef HASHKEYS_H
#define HASHKEYS_H

#include <random>
#include "types.hpp"
#include "move.hpp"

#define MB 0x100000

// Hash flags
enum HashFlag {
    
    NOHASH, EXACTHASH, ALPHAHASH, BETAHASH
    
};

// Hash entry
typedef struct {
    
    uint64_t key;
    int depth;
    int flag;
    int score;
    Move bestmove;
    
} TTEntry;

typedef struct {
    
    uint64_t key;
    Score score;
    uint64_t pawnWAttacks;
    uint64_t pawnBAttacks;
    uint64_t passedPawns;
    uint64_t pawnWAttacksSpan;
    uint64_t pawnBAttacksSpan;    
    
} PawnEntry;

typedef struct {
    
    uint64_t key;
    Score score;

} MaterialEntry;

// Transposition Table class
class TranspositionTable {
    
    public:

        int size;
        
        TTEntry *table;
        
        void setSize(const int hashsize);
        void clear();
        TTEntry * probe(const uint64_t key);
        void store(const uint64_t key, const int depth, const int score, const Move bestmove, const int flag);
        
        ~TranspositionTable() {
            delete[] table;
        }
};

class PawnTable {
    
    public:
    
        int size = 0x10000;
        
        PawnEntry *table;
        
        void clear();
        PawnEntry * probe(const uint64_t key);
        void store(const uint64_t key, const Score score, const uint64_t pawnWAttacks, const uint64_t pawnBAttacks, const uint64_t passedPawns, const uint64_t pawnWAttacksSpan, const uint64_t pawnBAttacksSpan);
        
        PawnTable() {
            table = new PawnEntry [size];
        }
        
        ~PawnTable() {
            delete[] table;
        }
    
};

class MaterialTable {
    
    public:
        
        int size = 0x2000;
        
        MaterialEntry *table;
        
        void clear();
        MaterialEntry * probe(const uint64_t key);
        void store(const uint64_t key, const Score score);
        
        MaterialTable() {
            table = new MaterialEntry [size];                        
        }
        
        ~MaterialTable() {
            delete[] table;
        }
    
};

// Generate a random 64bit integer
static const uint64_t getRandomInteger() {
    
    return rand() ^ ((uint64_t)rand() << 15) ^ ((uint64_t)rand() << 30) ^ ((uint64_t)rand() << 45) ^ ((uint64_t)rand() << 60);
    
}

// Random keys for each piece on every possible position
static const uint64_t pieceHashKeys[16][64] = {

    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },   
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() }
    
};

static const uint64_t pawnHashKeys[2][64] = {
    
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() }
    
};

static const uint64_t materialHashKeys[14][11] = {
    
    { 0 },
    { 0 },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() },
    { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() }

};

// Random turn, castling and enPassant Keys
static const uint64_t turnHashKeys[2] = { getRandomInteger(), getRandomInteger() };

static const uint64_t castlingHashKeys[16] = { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() };

static const uint64_t enPassantHashKeys[9] = { getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger(), getRandomInteger() };

#endif