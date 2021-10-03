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

#include "timeman.hpp"
#include "uci.hpp"

// Get the total time elapsed in milliseconds since a given timestamp
Duration get_time_elapsed(TimePoint start) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
}

// Initialize the time management given the search limits specified
void init_time_management(SearchInfo* info) {

    // We have two variables, idealTime and maxTime
    // Ideally, we want to search for about as long as the value of idealTime
    // However, if the principal variation or the score change often,
    // we extend the time we take to search the position. We will never go beyond
    // the maxTime value though
    if (info->limits.moveTime) { // Specific move time
        info->idealTime = info->limits.moveTime;
        info->maxTime   = info->limits.moveTime;
    } else if (info->limits.time) {
        info->idealTime = 1 * (info->limits.time + 25 * info->limits.increment) / 50;
        info->maxTime   = 5 * (info->limits.time + 25 * info->limits.increment) / 50;
    } else {
        return; // No time limit
    }

    // Subtract the Move Overhead from the remaining time
    info->idealTime = std::min(info->idealTime - MoveOverheadOption.get_value(), info->idealTime);
    info->maxTime   = std::min(info->maxTime   - MoveOverheadOption.get_value(), info->maxTime);

}

// Updates the time management. Called at the end of every search iteration
void update_time_management(SearchInfo* info) {

    if (info->depth > 5) {

        const Value value     = info->value[info->depth];
        const Value lastValue = info->value[info->depth - 1];

        // Increase the time we take to search if the value decreases across iterations
        if (lastValue > value + 10) info->idealTime *= 1.025;
        if (lastValue > value + 20) info->idealTime *= 1.025;
        if (lastValue > value + 40) info->idealTime *= 1.025;

        // If the value increases, we will increase the time as well
        if (lastValue + 15 < value) info->idealTime *= 1.015;
        if (lastValue + 30 < value) info->idealTime *= 1.025;

        // Update the pv stability. If the best move is constant, it will go to zero
        info->pvStability = std::max(0, info->pvStability - 1);
        if (info->bestMove[info->depth-1] != info->bestMove[info->depth]) {
            info->pvStability = 8;
        }
    }

}

// Check if we have exceeded the maximum allocated time for the search
bool is_time_exceeded(const SearchInfo* info) {

    return get_time_elapsed(info->start) >= info->maxTime;

}

// Check if we should stop the search and return the best move or continue with another iteration
bool should_stop(const SearchInfo& info) {

    if (!info.limits.time && !info.limits.moveTime) {
        return false;
    }

    const Duration elapsed = get_time_elapsed(info.start);
    // If the pv is unstable accross iterations, increase the time for searching
    const Duration ideal = info.idealTime * (1 + info.pvStability * 0.05);
    return elapsed >= std::min(ideal, info.maxTime);
    
}
