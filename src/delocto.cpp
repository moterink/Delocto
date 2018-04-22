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

#include "types.hpp"
#include "movegen.hpp"
#include "evaluate.hpp"
#include "uci.hpp"
#include "magic.hpp"
#include "bitboards.hpp"

#define VERSION 0.6

int main() {
    
    std::string input;
    
    // Generate occupancy variations and magic bitboards for fast
    // generation of bishop and rook moves
    generateOccupancyVariations(true);
    generateMoveDatabase(true);
    generateOccupancyVariations(false);
    generateMoveDatabase(false);
    
    // Init Bitboard Lookup Tables
    initBitboards();
    
    // Initialize king distance array
    initKingDistance();
    
    // Initialize Piece Square Tables
    initPSQT();
    
    // Initialize Eval
    initEval();
    
    std::cout << "Delocto " << VERSION << " by Moritz Terink" << std::endl << std::endl;
    
    uciloop();
    
    return 0;
    
}