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

#ifndef THREAD_H
#define THREAD_H

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "search.hpp"

class Thread {

    private:

        void clear_killers();
        void clear_history();

    public:

        Board board;

        PawnTable pawnTable;
        MaterialTable materialTable;

        Move killers[DEPTH_MAX + 1][2];
        Move counterMove[2][7][64];
        int history[2][7][64];

        Thread(const unsigned threadIndex);
        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;

        unsigned get_index() { return index; }

        void initialize(const Board& b, const SearchLimits& searchLimits);
        void clear();
        void destroy();
        void idle();
        void wait();
        void start();
        void stop();
        void search();
        uint64_t get_nodes() { return info.nodes; };

    private:

        unsigned index;
        bool isSearching = false;
        bool shouldExit = false;

        std::mutex mtx;
        std::condition_variable cv;
        
        SearchLimits limits;
        SearchInfo info;

};

class ThreadPool {

    public:
        
        unsigned get_thread_count() { return threads.size(); }
        Thread* get_thread(const unsigned index) { return threads[index]; }

        ThreadPool(const unsigned count);
        void resize(const unsigned threadCount);
        void reset();
        void initialize_search(const Board& board, const SearchLimits& limits);
        void start_searching();
        void stop_searching() { stopped = true; }
        void wait_until_finished();
        bool has_stopped() { return stopped; }
        uint64_t get_nodes();

    private:
    
        std::vector<Thread*> threads;

        std::atomic_bool stopped = true;

};

#endif