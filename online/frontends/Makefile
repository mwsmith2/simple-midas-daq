#####################################################################
#
#  Name:         Makefile
#  Created by:   Matthias Smith
#
#  Contents:     Makefile for MIDAS NMR frontends, based on
#                a sample Makefile written by Stefan Ritt.
#
#####################################################################
BIN_DIR = bin

COREOBJ = $(patsubst core/src/%.cxx, core/build/%.o, $(wildcard core/src/*.cxx))
DEPSOBJ = $(patsubst core/src/drs/%.c, core/build/%.o, $(wildcard core/src/drs/*.c))
DEPSOBJ += $(patsubst core/src/drs/%.cpp, core/build/%.o, $(wildcard core/src/drs/*.cpp))
DEPSOBJ += $(patsubst core/include/vme/%.c, core/build/%.o, $(wildcard core/include/vme/*.c))
SRC_VXI = src/vxi/vxi11_clnt.cc src/vxi/vxi11_xdr.cc include/vxi/vxi11.h src/vxi/vxi11_user.cc
DEPSOBJ += build/vxi11_clnt.o build/vxi11_xdr.o build/scope_reader.o build/vxi11_user.o

FRONTENDS = $(patsubst src/%.cxx,$(BIN_DIR)/%,$(wildcard src/fe*.cxx))
ANALYZERS = $(patsubst src/%.cxx,$(BIN_DIR)/%,$(wildcard src/an*.cxx))
UTILITIES = $(patsubst src/%.cxx,$(BIN_DIR)/%,$(wildcard src/vme*.cxx))

#-----------------------------------------
# This is for Linux
OSFLAGS = -DOS_LINUX -DHAVE_USB -DHAVE_LIBUSB10 -Dextname
CFLAGS = -g -fpermissive
CXXFLAGS = -std=c++0x 
LIBS = -lm -lz -lpthread -lfftw3 -lzmq -lcurl -lodbc -lrt

# ROOT flags and libs
ROOTFLAGS = $(shell root-config --cflags)
ROOTLIBS = $(shell root-config --libs)

# wxWidgets libs and flags
WXLIBS  = $(shell wx-config --libs)
WXFLAGS = $(shell wx-config --cxxflags)

#-------------------------------------------------------------------
# The following lines define direcories. Adjust if necessary
INC_DIR = $(MIDASSYS)/include
LIB_DIR = $(MIDASSYS)/linux/lib

# Libraries && include flags
LIB = -L$(LIB_DIR) -Lpkg/lib -L/usr/lib64/mysql -Lcore/lib -luwlab 
LIB += -lmidas-shared -lfid -lmysqlclient -lsqlite3 -lcurl -ljson_spirit
LIB += -lCAENDigitizer -lusb-1.0 -lutil
CFLAGS += -I$(INC_DIR) -Iinclude -Icore/include -Icore/include/drs -Iinclude/vxi
CFLAGS += -Wl,-rpath,/usr/local/lib

# Set compilers
CC = gcc
CXX = g++ -std=c++11

#-------------------------------------------------------------------
# Finally we have the actual make directives.

.SECONDARY: $(OBJECTS)

.PHONY: print_vars

# Make commands

all: $(FRONTENDS) $(ANALYZERS) $(UTILITIES)

print_vars:
	echo $(FRONTENDS)
	echo $(ANALYZERS)

$(BIN_DIR)/fe_%: src/fe_%.cxx $(LIB_DIR)/mfe.o $(COREOBJ) $(DEPSOBJ)
	$(CXX) -o $@ $+ $(CXXFLAGS) $(CFLAGS) $(OSFLAGS) $(WXFLAGS) \
	$(ROOTFLAGS) $(LIB) $(LIBS) $(ROOTLIBS) $(WXLIBS)

$(BIN_DIR)/an_%: src/an_%.cxx $(LIB_DIR)/rmana.o $(COREOBJ) $(DEPSOBJ)
	$(CXX) -o $@ $+ $(CXXFLAGS) $(CFLAGS) $(WXFLAGS) $(ROOTFLAGS) \
	$(LIB) $(LIBS) $(ROOTLIBS) $(WXLIBS)

$(BIN_DIR)/vme_%: src/vme_%.cxx $(COREOBJ) $(DEPSOBJ)
	$(CXX) -o $@ $+ $(CFLAGS) $(OSFLAGS) $(WXFLAGS) $(ROOTFLAGS) \
	$(LIB) $(LIBS) $(ROOTLIBS) $(WXLIBS)

core/build/%.o: core/src/%.cxx
	cd core && make && cd ..

build/%.o: src/vxi/%.cc
	$(CXX) -c $< -o $@ $(CFLAGS)

$(SRC_VXI): src/vxi/vxi11.x
	mv $+ .
	rpcgen -M -c -o vxi11_xdr.cc vxi11.x
	rpcgen -M -h -o vxi11.h vxi11.x
	rpcgen -M -l -o vxi11_clnt.cc vxi11.x
	mv vxi11*.cc src/vxi
	mv vxi11*.h include/vxi
	mv vxi11.x src/vxi

clean:
	cd core && make clean && cd ..; \
	rm -f *~ $(OBJECTS) $(FRONTENDS) $(ANALYZERS)
