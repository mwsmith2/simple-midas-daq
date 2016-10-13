# Grab the targets and sources as two batches
SOURCES = $(wildcard src/*.cxx)
OBJECTS = $(patsubst src/%.cxx,build/%.o,$(SOURCES))
TARGETS = $(patsubst src/%.cxx,bin/%,$(SOURCES))

# Debugging/profiling flags
# DEBUG = -g -pg

# Figure out the architecture
UNAME_S = $(shell uname -s)

# Clang compiler
ifeq ($(UNAME_S), Darwin)
	CXX = clang++
	CC  = clang
	FLAGS = -std=c++11 $(DEBUG)
endif

# Gnu compiler
ifeq ($(UNAME_S), Linux)
	CXX = g++
	CC  = gcc
	FLAGS = -std=c++0x $(DEBUG)
endif

LIBS = -lfftw3 -lm -lboost_filesystem -lboost_system -larmadillo -lfid

FLAGS += $(shell root-config --cflags) -Iinclude -Icore/include -Lcore/lib
LIBS  += $(shell root-config --libs)

all: $(TARGETS)

bin/%: src/%.cxx
	$(CXX) $(FLAGS) -o $@ $< $(LIBS)

clean:
	rm -f $(OBJECTS) $(TARGETS)
