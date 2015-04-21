#
# Makefile for sstring.
#
# e.g.
# Build with defaults: make
# Build with gcc and profiling: make CC=gcc PROFILING=1
# Build with gcc using C89/90 standard: make CC=gcc C99=0
# Build with g++ default C++ settings: make CC=g++
# Build with clang++ using C++11 standard: make CC=clang++ CPP11=1
# Build with TinyCC with debug symbols: make CC=tcc DEBUG=1
# Build with gcc cross compiler: make CC=powerpc-linux-gnu-gcc
#
# Copyright (c) 2015 F. Aragon. All rights reserved.
#

# Default flag (can be overriden in command line):
ifndef C99
	C99=0
endif
ifndef CPP11
	CPP11=0
endif
ifndef PROFILING
	PROFILING=0
endif
ifndef DEBUG
	DEBUG=0
endif
ifndef PEDANTIC
	PEDANTIC=0
endif
ifndef MINIMAL_BUILD
	MINIMAL_BUILD=0
endif
ifndef FORCE32
	FORCE32=0
endif

# Configure compiler context:
ifeq ($(CC), tcc)
	PROFILING=0
else
	CPP_MODE=0
	ifeq ($(CC), g++)
		CPP_MODE=1
	endif
	ifeq ($(CC), clang++)
		CPP_MODE=1
	endif
	ifeq ($(CPP_MODE), 1)
		ifeq ($(CPP11), 1)
			CFLAGS += -std=c++11
		endif
	else
		ifeq ($(C99), 1)
			CFLAGS += -std=c99
		else
			CFLAGS += -std=c89
		endif
	endif
	ifeq ($(PEDANTIC), 1)
		CFLAGS += -pedantic
	endif
endif
ifeq ($(DEBUG), 1)
	CFLAGS += -O0 -ggdb -DS_DEBUG
else
	ifeq ($(MINIMAL_BUILD), 1)
		CFLAGS += -Os -DS_MINIMAL_BUILD
	else
		CFLAGS += -O2
	endif
endif
ifeq ($(PROFILING), 1)
	CFLAGS += -ggdb
	# gcov flags:
	CFLAGS += -fprofile-arcs -ftest-coverage
	LDFLAGS += -lgcov -coverage
	# gprof flags:
	CFLAGS += -pg
	LDFLAGS += -pg
endif
ifeq ($(FORCE32), 1)
	CFLAGS += -m32
	CXXFLAGS += -m32
	LDFLAGS += -m32
endif

DATE    = $(shell date)
SOURCES	=sdata.c sdbg.c senc.c sstring.c schar.c ssearch.c svector.c stree.c smap.c
HEADERS	=scommon.h $(SOURCES:.c=.h)
OBJECTS	=$(SOURCES:.c=.o)
LIBSHLL	=shll.a
TEST	=stest
EXES	=$(TEST) sbench

# Rules:
all: $(EXES) run_tests
$(OBJECTS): $(HEADERS)
$(LIBSHLL): $(OBJECTS)
	ar rcs $(LIBSHLL) $(OBJECTS)
$(EXES): $% $(LIBSHLL)
run_tests:
	@./$(TEST)
clean:
	@rm -f $(OBJECTS) $(LIBSHLL) *\.gcno *\.gcda *\.out callgrind* out\.txt
	@for X in $(EXES) ; do rm -f $$X $$X.o ; done

