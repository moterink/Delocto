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

#include <set>

enum OptionType {
    Check, Spin, Combo, Button, String
};

class Option {

    public:
        std::string name;
        OptionType type;

        Option(const std::string& n, OptionType t): name(n), type(t) {}

        virtual std::string uci_string() const {
            return "option name " + name + " type ";
        };

};

class CheckOption : public Option {

    private:
        bool value;
        bool defaultValue;

    public:
        CheckOption(const std::string& name, bool initialValue) : Option(name, Check) {
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

        std::string uci_string() const override {
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
        SpinOption(const std::string& name, int initialValue, int min, int max) : Option(name, Spin) {
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

        std::string uci_string() const override {
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
        const std::set<std::string> comboValues;

    public:
        ComboOption(const std::string& name, const std::string& initialValue, std::initializer_list<std::string> choices) : Option(name, Combo), value(initialValue), defaultValue(initialValue), comboValues{choices} {}

        bool set_value(const std::string& newValue) {
            if (comboValues.find(newValue) != comboValues.end()) {
                value = newValue;
                return true;
            }
            return false;
        }

        std::string get_value() const {
            return value;
        }

        std::string get_default() const {
            return defaultValue;
        }

        std::string uci_string() const override {
            std::stringstream ss;
            ss << Option::uci_string() << "combo default " << defaultValue;
            for (const std::string& choice : comboValues) {
                ss << " var " << choice;
            }
            return ss.str();
        }
};

class ButtonOption : public Option {

    private:
        std::function<void ()> callback;

    public:
        ButtonOption(const std::string& name, std::function<void ()> c) : Option(name, Button), callback(c) {}

        void push() {
            callback();
        }

        std::string uci_string() const override {
            return Option::uci_string() + "button";
        }

};

class StringOption : public Option {

    private:
        std::string value;
        std::string defaultValue;

    public:
        StringOption(const std::string& name, const std::string& initialValue): Option(name, String), value(initialValue), defaultValue(initialValue) {}

        bool set_value(const std::string& newValue) {
            value = newValue;
            return true;
        }

        std::string get_value() const {
            return value;
        }

        std::string get_default() const {
            return defaultValue;
        }

        std::string uci_string() const override {
            return Option::uci_string() + "string default " + defaultValue;
        }

};

extern SpinOption ThreadsOption;
extern SpinOption HashOption;
extern SpinOption MoveOverheadOption;

extern ThreadPool Threads;
extern TranspositionTable TTable;

namespace UCI {
    extern void init();
    extern void loop(int argc, char* argv[]);
    extern void send_pv(const SearchInfo& info, const Value value, const PrincipalVariation& pv, const uint64_t nodes, const Value alpha, const Value beta);
    extern void send_currmove(const Move currentMove, const unsigned index);
    extern void send_bestmove(const Move bestMove);
    extern void send_string(const std::string& string);
    extern void go(const Board& board, const SearchLimits& limits);
}

#endif
