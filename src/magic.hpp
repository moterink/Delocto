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

#ifndef MAGIC_H
#define MAGIC_H

#include "types.hpp"

// Magic numbers
extern const uint64_t magicNumberRook[64];
extern const uint64_t magicNumberBishop[64];

// Occupancy masks
extern const uint64_t occupancyMaskRook[64];
extern const uint64_t occupancyMaskBishop[64];

// Magic shifts
extern const int magicNumberShiftsRook[64];
extern const int magicNumberShiftsBishop[64];

// Occupancy Variation
extern uint64_t occupancyVariation[64][4096];

// Move databases
extern uint64_t magicMovesRook[64][4096];
extern uint64_t magicMovesBishop[64][4096];

extern void generateOccupancyVariations(bool isRook);
extern void generateMoveDatabase(bool isRook);

#endif
