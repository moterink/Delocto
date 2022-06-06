# Delocto Chess Engine
# Copyright (c) 2018-2021 Moritz Terink

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

NAME := delocto
CXXSTD := -std=c++17
SRC := *.cpp

OPTIMIZEFLAGS = -DNDEBUG -O3 -pthread
DEBUGFLAGS = -g
WARNFLAGS = -Wall -Wextra

define init
@echo Detected $(ARCH) architecture
endef

define compile
$(CXX) $(CXXSTD) $(1) $(WARNFLAGS) $(SRC) -o $(NAME)
endef

define finish
@echo Built executable named \"$(NAME)\".
@echo Finished.
endef

ifneq ($(OS),Windows_NT)
OS := $(shell uname -s)
ARCH = $(shell uname -p)
endif

# ARM architecture prefers -mcpu over -march (some ARM processors don't support -march at all)
ifeq ($(ARCH),arm)
OPTIMIZEFLAGS += -mcpu=native
else
OPTIMIZEFLAGS += -march=native
endif

optimized:
	$(call init)
	@echo Creating optimized build...
	$(call compile,$(OPTIMIZEFLAGS))
	$(call finish)

debug:
	$(call init)
	@echo Creating debug build...
	$(call compile,$(DEBUGFLAGS))
	$(call finish)
