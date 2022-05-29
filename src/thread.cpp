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

#include "thread.hpp"
#include "uci.hpp"

ThreadPool::ThreadPool(const unsigned count) {

    for (unsigned i = 0; i < count; i++) {
        threads.push_back(new Thread(i));
    }

}

// Resize the Thread Pool
void ThreadPool::resize(const unsigned count) {

    const int difference = std::max(1u, count) - get_thread_count();

    // If the number of thread should decrase, destroy any excess threads
    if (difference < 0) {
        for (int i = 0; i < std::abs(difference); i++) {
            Thread* thread = threads.back();
            thread->destroy();
            delete thread;
            threads.pop_back();
        }
    // If we need extra threads, create them
    } else if (difference > 0) {
        for (int i = 0; i < difference; i++) {
            unsigned threadIndex = get_thread_count();
            threads.push_back(new Thread(threadIndex));
        }
    }

}

// Reset all threads
void ThreadPool::reset() {

    for (unsigned i = 0; i < get_thread_count(); i++) {
        threads[i]->clear();
    }

}

// Initialize a new search
void ThreadPool::initialize_search(const Board& board, const SearchLimits& limits) {

    // Reset stop flag and increase transposition table age
    stopped = false;
    TTable.new_search();

    for (unsigned i = 0; i < get_thread_count(); i++) {
        threads[i]->initialize(board, limits);
    }

}

// Wake up idle threads and make them search
void ThreadPool::start_searching() {

    // Start helper threads
    for (unsigned i = 1; i < get_thread_count(); i++) {
        threads[i]->start();
    }
    // Start main thread
    threads[0]->start();

}

// Wait until every thread has finished searching
void ThreadPool::wait_until_finished() {

    for (unsigned i = 0; i < get_thread_count(); i++) {
        threads[i]->wait();
    }

}

// Calculate the cumulative number of nodes searched across all threads
uint64_t ThreadPool::get_nodes() {

    uint64_t nodes = 0;

    for (unsigned i = 0; i < get_thread_count(); i++) {
        nodes += threads[i]->get_nodes();
    }

    return nodes;

}

// Create a new thread
Thread::Thread(const unsigned threadIndex) {

    index = threadIndex;

    // The thread starts waiting in the thread pool for a search request
    std::thread thread = std::thread(&Thread::idle, this);
    thread.detach();

}

// Set search limits and the current position
void Thread::initialize(const Board& b, const SearchLimits& limits) {

    board = b;
    info.reset();
    info.limits = limits;

    // TODO: This seems to positively affect strength - why??
    killers.clear();
    history.clear();
    counterMove.clear();

}

// Reset board, killers, history, info and hash tables for thread
void Thread::clear() {

    board = Board();
    pawnTable.clear();
    materialTable.clear();
    killers.clear();
    history.clear();
    counterMove.clear();
    info.reset();

}

// Make the thread start searching
void Thread::start() {

    assert(!isSearching);

    std::lock_guard<std::mutex> lck(mtx);
    isSearching = true;
    cv.notify_one(); // Wake up thread in idle loop

}

// Called when the thread finished searching
void Thread::stop() {

    isSearching = false;
    cv.notify_one(); // Notify anyone waiting for the thread to finish searching

}

// Destroy this thread
void Thread::destroy() {

    shouldExit = true;
    cv.notify_one(); // Make thread in idle loop exit

}

void Thread::idle() {

    while (true) {

        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [this] { return isSearching || shouldExit; });

        if (shouldExit) {
            return;
        }

        search();

        stop();

    }

}

// Wait for this thread to finish searching
// Used for waiting for a result from this thread
void Thread::wait() {

    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck, [this] { return !isSearching; });

}