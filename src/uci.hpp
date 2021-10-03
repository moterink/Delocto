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

#ifndef UCI_H
#define UCI_H

#include "types.hpp"
#include "hashkeys.hpp"
#include "move.hpp"
#include "search.hpp"
#include "thread.hpp"
#include "perft.hpp"

enum OptionType {
    Check, Spin, Combo, Button, String
};

class Option {

    public:
        std::string name;
        OptionType type;

        Option(std::string& n, OptionType t) {
            name = n;
            type = t;
        }

        virtual std::string uci_string() const {
            return "option name " + name + " type ";
        };

};

class CheckOption : public Option {

    private:
        bool value;
        bool defaultValue;

    public:
        CheckOption(std::string name, bool initialValue) : Option(name, Check) {
            value = initialValue;
            defaultValue = initialValue;
        }

        bool set_value(bool newValue) {
            value = newValue;
            return true;
        }

        bool get_value() const {
            return value;
        }

        std::string uci_string() const {
            return Option::uci_string() + "check default " + std::to_string(defaultValue);
        }

};

class SpinOption : public Option {

    private:
        int value;
        int defaultValue;
        int minValue;
        int maxValue;

    public:
        SpinOption(std::string name, int initialValue, int min, int max) : Option(name, Spin) {
            value = initialValue;
            defaultValue = initialValue;
            minValue = min;
            maxValue = max;
        }

        bool set_value(int newValue) {
            if (newValue < minValue || newValue > maxValue) {
                return false;
            }
            value = newValue;
            return true;
        }

        int get_value() const {
            return value;
        }

        int get_default() const {
            return value;
        }

        int get_min() const {
            return minValue;
        }

        int get_max() const {
            return maxValue;
        }

        std::string uci_string() const {
            return Option::uci_string()
                   + "spin default " + std::to_string(defaultValue)
                   + " max " + std::to_string(maxValue)
                   + " min " + std::to_string(minValue);
        }

};

class ComboOption : public Option {

    private:
        std::string value;
        std::string defaultValue;
        const std::vector<std::string> comboValues;

    public:
        ComboOption(std::string name, const std::string initialValue, std::initializer_list<std::string> choices) : Option(name, Combo), comboValues{choices} {
            value = initialValue;
            defaultValue = initialValue;
        }

        bool set_value(std::string newValue) {
            for (const std::string comboValue : comboValues) {
                if (comboValue == newValue) {
                    value = newValue;
                    return true;
                }
            }
            return false;
        }

        std::string get_value() const {
            return value;
        }

        std::string get_default() const {
            return defaultValue;
        }

        std::string uci_string() const {
            std::stringstream ss;
            ss << Option::uci_string() << "combo default " << defaultValue;
            for (const std::string choice : comboValues) {
                ss << " var " << choice;
            }
            return ss.str();
        }
};

class ButtonOption : public Option {

    private:
        std::function<void ()> callback;

    public:
        ButtonOption(std::string name, std::function<void ()> c) : Option(name, Button) {
            callback = c;
        }

        void push() {
            callback();
        }

        std::string uci_string() const {
            return Option::uci_string() + "button";
        }

};

class StringOption : public Option {

    private:
        std::string value;
        std::string defaultValue;

    public:
        StringOption(std::string name, std::string initialValue): Option(name, String) {
            value = initialValue;
            defaultValue = initialValue;
        }

        bool set_value(std::string newValue) {
            value = newValue;
            return true;
        }

        std::string get_value() const {
            return value;
        }

        std::string get_default() const {
            return defaultValue;
        }

        std::string uci_string() const {
            return Option::uci_string() + "string default " + defaultValue;
        }

};

// Testing positions for benchmark
static std::string BENCHMARK_FENS[42] = {

    "2kr1b1r/1pp2pp1/p1n1bq2/P2pp3/1P2Pn1p/2PP1N1P/1BQN1PP1/R3KB1R b KQ - 2 12",
    "r4rk1/ppqb2pp/n2bp3/5p2/3B4/2PB1N2/PPQ2PPP/3RK2R b K - 0 12",
    "4r2k/p4rpp/1pb5/5p2/7q/2PR1P2/PPQ1BP2/5KR1 w - - 2 23",
    "r4rk1/ppqb2pp/n2bp3/2p2p2/3P4/2PBBN2/PPQ2PPP/R3K2R w KQ - 2 11",
    "r1bqk2r/1pp1pp2/3p1n1p/pPn3p1/2P5/2P1P2P/P2N1PP1/R1BQKB1R w KQkq - 5 8",
    "3r1r2/2p1ppk1/1p2qnbp/pPp3p1/2P5/P1P1P2P/3NBPP1/2R1QRK1 b - - 2 17",
    "r7/2p2pk1/1p4b1/1Pp4p/2P5/2K1PN2/p5P1/R7 w - - 0 41",
    "r1bq3r/3nnkp1/2pbpp2/p2p4/P4P2/2N1PN1p/1BPPBRPP/R2Q2K1 w - - 0 12",
    "2r2rk1/p2q2p1/2R1p3/8/Q2PpP1p/6P1/PP3P1P/2R3K1 b - - 0 21",
    "1r1r2k1/2p1qppp/2p5/p1bpP3/N7/2P3Q1/PPR2PPP/3R2K1 b - - 0 21",
    "r4rk1/ppb2ppp/3q1B2/3pp3/P5b1/1BNP1n2/1PP2PPP/R2Q1RK1 w - - 0 14",
    "r3k3/1pp1np1p/p4br1/8/4R3/2N2NP1/PP3P1P/R1B3K1 b q - 2 18",
    "4k3/4R3/p1p2pNP/1p3rn1/8/4B1P1/PP3P2/5K2 b - - 12 42",
    "3k4/6b1/1R6/pPP1p2p/b5p1/5r2/4K3/8 b - - 3 51",
    "1rq1kb1r/4p3/p1p2p2/3p2Pp/2nP2b1/3NB1N1/PPP2QPP/1R3RK1 b k - 0 19",
    "8/6k1/p5P1/3P1b2/1N1P1Q2/1P5p/Pn5P/5RK1 w - - 1 40",
    "r1q1kb1r/4pp2/p1p2n1p/3pNbp1/3P4/4B3/PPP1NPPP/R2Q1RK1 b kq - 3 12",
    "r2qr1k1/1ppb1ppp/2p2n2/p1b1p3/4P3/2NP1N1P/PPP2PP1/R1BQR1K1 w - - 0 10",
    "r2qr1k1/1bp4p/1p4p1/p1p1p2n/P1N1Pp2/1PNP1QPP/2P2P2/R3R1K1 w - - 3 23",
    "8/1Q4kp/6p1/P3p1nb/1p1qP1r1/8/2Pp1P2/3R1R1K b - - 1 48",
    "2r3k1/5p1p/1p2p1p1/1P1pP1P1/q1rP4/p1P1Q2P/2RK1P2/1R6 w - - 4 39",
    "6k1/5p1p/4p1p1/3pP1P1/8/2r2PKP/p3q3/Q7 w - - 0 56",
    "8/5p1k/4p1pP/3pP3/7K/5P1P/8/6q1 w - - 4 65",
    "2r2rk1/p1q2p1p/1pb1p1p1/3pP1P1/P2P1Q2/1RPB1K1P/5P2/R7 b - - 0 30",
    "r4rk1/1pp3np/p1p4Q/P4bN1/2qbNB2/6Pp/1P6/R3R2K w - - 0 25",
    "rn1qr1k1/pp3ppp/2pb4/5b2/3Pp3/2N1PP2/PP1N2PP/R1BQR1K1 b - - 0 12",
    "2k5/1b6/1P6/b3p2p/8/2r5/K7/2q5 b - - 3 64",
    "6k1/4npb1/1p1p2pp/3Pp3/1Q2P3/5NP1/1P2NPKP/2R5 b - - 3 31",
    "6R1/6PR/8/p5k1/P2p4/3B4/1Pp4P/2K1r3 w - - 1 41",
    "r1bqk2r/pppp1ppp/2nb1n2/1B2p3/4P3/2N2N2/PPPP1PPP/R1BQK2R w KQkq - 6 5",
    "rnbqk2r/pppp1Bpp/5n2/2b1p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 4",
    "2k5/P7/1K6/8/8/8/8/8 w - - 0 1",
    "8/8/8/8/8/5K2/7p/5k2 w - - 0 1",
    "r3kr2/8/6b1/8/8/8/8/R3K2R w KQq - 0 1",
    "r3k2r/8/8/8/8/6B1/8/R3KR2 w Qkq - 0 1",
    "r1b1k1nr/8/8/8/8/8/8/R1B1K1NR w KQkq - 0 1",
    "r3k2r/8/8/1B6/8/8/8/R3K2R b KQkq - 0 1",
    "r3k2r/8/8/8/1b6/8/8/R3K2R w KQkq - 0 1",
    "r3k2r/8/8/8/5b2/5n2/8/R3K2R w KQkq - 0 1",
    "r3k2r/8/5N2/5B2/8/8/8/R3K2R b KQkq - 0 1",
    "rnbqkbnr/p3pp1p/2p5/1p3Pp1/3P4/2N5/PPP3PP/R1BQKBNR w KQkq g6 0 5",
    "rnbqkbnr/p1pp1ppp/4p3/8/1pP1P3/5N2/PP1PBPPP/RNBQK2R b KQkq c3 0 3"

};

static const std::string PromotionChar = "nbrq";

// Converts a promotion character to a MoveType
inline MoveType char_to_promotion(const char c) {

    return (PromotionChar.find(c) * 2 + 1) * PROMOTION_KNIGHT;

}

// Converts a MoveType e.g PROMOTION_QUEEN to a character, e.g q, r, b, n
inline char promotion_to_char(const MoveType mt) {

    return PromotionChar[(mt / PROMOTION_KNIGHT) / 2];

}

// Converts a move object to a string, e.g "a2a5"
inline std::string move_to_string(const Move raw) {

    return (std::string() + SQUARE_NAMES[from_sq(raw)] + SQUARE_NAMES[to_sq(raw)]) + (is_promotion(raw) ? std::string(1, promotion_to_char(move_type(raw))) : "");

}

extern SpinOption HashOption;
extern SpinOption MoveOverheadOption;

extern ThreadPool Threads;
extern TranspositionTable TTable;

extern uint64_t benchmark();
extern void send_pv(const SearchInfo& info, const Value value, const PrincipalVariation& pv, const uint64_t nodes, const Value alpha, const Value beta);
extern void send_currmove(const Move currentMove, const unsigned index);
extern void send_bestmove(const Move bestMove);
extern void send_string(const std::string string);
extern void uci_loop(int argc, char* argv[]);

#endif
