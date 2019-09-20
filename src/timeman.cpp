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

#include "timeman.hpp"

int MoveOverhead = 100;

long long get_time_elapsed(TimePoint start) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
};

void init_time_management(const SearchLimits& limits, SearchInfo* info) {

    if (limits.moveTime >= 0) {
        info->idealTime = limits.moveTime;
        info->maxTime   = limits.moveTime;
    } else if (limits.time >= 0) {
        info->idealTime = 1 * (limits.time + 25 * limits.increment) / 50;
        info->maxTime   = 5 * (limits.time + 25 * limits.increment) / 50;
    } else {
        info->limitTime = false;
    }

    info->idealTime = std::min(limits.time - MoveOverhead, info->idealTime);
    info->maxTime   = std::min(limits.time - MoveOverhead, info->maxTime);

}

void update_time_managememnt(SearchInfo* info) {

    const long long elapsed = get_time_elapsed(info->start);

    if (info->depth > 5) {

        const int value = info->value[info->depth];
        const int lastValue = info->value[info->depth - 1];

        if (lastValue > value + 10) info->idealTime *= 1.025;
        if (lastValue > value + 20) info->idealTime *= 1.025;
        if (lastValue > value + 40) info->idealTime *= 1.025;

        if (lastValue + 15 < value) info->idealTime *= 1.015;
        if (lastValue + 30 < value) info->idealTime *= 1.025;

        info->pvStability = std::max(0, info->pvStability - 1);
        if (info->bestmove[info->depth-1] != info->bestmove[info->depth]) {
            info->pvStability = 8;
        }
    }

}

bool is_time_exceeded(const SearchInfo* info) {

    return get_time_elapsed(info->start) >= info->maxTime;

}

bool should_stop(const SearchInfo& info) {

    const long long elapsed = get_time_elapsed(info.start);
    const long long ideal = info.idealTime * (1 + info.pvStability * 0.05);
    return elapsed >= std::min(ideal, info.maxTime);

}
