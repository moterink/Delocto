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
#include <utility>

#include "./catch.hpp"

#include "../src/perft.hpp"

// Run multiple perfts to ensure move generator legality
TEST_CASE("Check perft results") {
    static const std::array<std::pair<std::string, Depth>, 4> fens = {
      std::make_pair("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5),
      std::make_pair("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4),
      std::make_pair("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5),
      std::make_pair("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4)
    };

    static const std::vector<uint64_t> results[] = {
      {
        20,
        400,
        8902,
        197281,
        4865609
      },
      {
        48,
        2039,
        97862,
        4085603,
      },
      {
        14,
        191,
        2812,
        43238,
        674624
      },
      {
        6,
        264,
        9467,
        422333,
      }
    };

    for (unsigned f = 0; f < fens.size(); f++) {
      const std::pair<std::string, Depth> fen = fens[f];
      DYNAMIC_SECTION("FEN: " << fen.first) {
        std::vector<uint64_t> result = runPerft(fen.first, fen.second);
        REQUIRE(result == results[f]);
      }
    }
}