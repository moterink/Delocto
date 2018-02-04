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

#ifndef UCI_H
#define UCI_H

#include "types.hpp"
#include "hashkeys.hpp"
#include "move.hpp"
#include "perft.hpp"

#define MAXHASHSIZE 160
#define MINHASHSIZE 0
#define DEFAULTHASHSIZE 160
#define MAXTHREADS 1
#define MINTHREADS 1
#define DEFAULTTHREADS 1

extern TranspositionTable tTable;
extern PawnTable pawnTable;
extern MaterialTable materialTable;

extern std::string move_to_string(const Move raw);
extern void showInformation();
extern void showSettings();
extern void uciloop();

#endif