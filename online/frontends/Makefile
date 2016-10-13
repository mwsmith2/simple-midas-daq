#####################################################################
#
#  Name:         Makefile
#  Created by:   Matthias Smith
#
#  Contents:     Makefile for MIDAS NMR frontends, based on
#		 a sample Makefile written by Stefan Ritt.
#
#####################################################################

NMR_DIR = core
OBJECTS = $(wildcard $(NMR_DIR)/build/*.o)
MODULES = $(patsubst src/%.cxx,%,$(wildcard src/*.cxx))

#-----------------------------------------
# This is for Linux
OSFLAGS = -DOS_LINUX -DHAVE_USB -DHAVE_LIBUSB10 -Dextname
CFLAGS = -g -fpermissive
CXXFLAGS = -std=c++0x 
LIBS = -lm -lz -lpthread -lfftw3 -lzmq -lodbc -lrt

ROOTFLAGS = $(shell root-config --cflags)
ROOTLIBS = $(shell root-config --libs)

#-------------------------------------------------------------------
# The following lines define direcories. Adjust if necessary
INC_DIR = $(MIDASSYS)/include
LIB_DIR = $(MIDASSYS)/linux/lib
BIN_DIR = bin
LINK_DIR = ../../common/bin

# wxWidgets libs and flags
WXLIBS  = $(shell wx-config --libs)
WXFLAGS = $(shell wx-config --cxxflags)

# Libraries && include flags
LIB = -L$(LIB_DIR) -L$(NMR_DIR)/lib -L/usr/lib64/mysql
LIB += -lmidas-shared -lmysqlclient -lsqlite3 -lCAENDigitizer -lusb-1.0
LIB += -ljson_spirit -lfid -luwlab -lcurl -lboost_system
CFLAGS += -I$(INC_DIR) -I$(NMR_DIR)/include -I$(NMR_DIR)/include/drs
CFLAGS += -Wl,-rpath,/usr/local/lib

# Set compilers
CC = gcc
CXX = g++

#-------------------------------------------------------------------
# Finally we have the actual make directives.

# Make commands

all: $(MODULES)

fe_%: src/fe_%.cxx $(LIB_DIR)/mfe.o
	$(CXX) -o $(BIN_DIR)/$@ $+ $(CXXFLAGS) $(CFLAGS) $(OSFLAGS) \
	$(LIB) $(LIBS) $(ROOTFLAGS) $(ROOTLIBS) $(WXFLAGS) $(WXLIBS)
	ln -fs $(shell pwd)/$(BIN_DIR)/$@ $(LINK_DIR)/$@ 

an_%: src/an_%.cxx $(LIB_DIR)/mana.o
	$(CXX) -o $(BIN_DIR)/$@ $+ $(CXXFLAGS) $(CFLAGS) $(OSFLAGS) \
	$(LIB) $(LIBS) $(ROOTFLAGS) $(ROOTLIBS) $(WXFLAGS) $(WXLIBS)
	ln -fs $(shell pwd)/$(BIN_DIR)/$@ $(LINK_DIR)/$@ 

clean:
	rm -f *~ $(MODULES)
