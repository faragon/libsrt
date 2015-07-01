#
# Makefile for sstring.
#
# Examples:
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

# Makefile parameters

ifndef C99
	C99=0
endif
ifndef C11
	C11=0
endif
ifndef CPP11
	CPP11=0
endif
ifndef CPP0X
	CPP0X=0
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

COMMON_FLAGS = -pipe

# Configure compiler context

UNAME = $(shell uname)
UNAME_M = $(shell uname -m)

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
	ifeq ($(CPP11), 1)
		CPP_MODE=1
	endif
	ifeq ($(CPP0X), 1)
		CPP_MODE=1
	endif
	ifeq ($(CPP_MODE), 1)
		ifeq ($(CPP11), 1)
			CFLAGS += -std=c++11
		endif
		ifeq ($(CPP0X), 1)
			CFLAGS += -std=c++0x
		endif
	else
		ifeq ($(C11), 1)
			CFLAGS += -std=c1x
		else
			ifeq ($(C99), 1)
				CFLAGS += -std=c99
			else
				CFLAGS += -std=c89
			endif
		endif
	endif
	ifeq ($(PEDANTIC), 1)
		ifneq (,$(findstring $(CC),gcc-g++))
			CFLAGS += -Wall -Wextra
		endif
		ifneq (,$(findstring $(CC),clang-clang++))
			CFLAGS += -Weverything
		endif
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
	COMMON_FLAGS += -pg
else
	COMMON_FLAGS += -fomit-frame-pointer
endif

OCTEON = 0
ifeq ($(UNAME_M), mips64)
	ifeq ($(UNAME), Linux)
		OCTEON = $(shell cat /proc/cpuinfo | grep cpu.model | grep Octeon | head -1 | wc -l)
	endif
endif

ifeq ($(FORCE32), 1)
	ifeq ($(UNAME_M), x86_64)
		COMMON_FLAGS += -m32
	endif
	ifeq ($(UNAME_M), mips64)
		ifeq ($(OCTEON), 1)
			COMMON_FLAGS += -mips64r2
		else
			COMMON_FLAGS += -mips32
		endif
	endif
else
	ifeq ($(UNAME_M), mips64)
		COMMON_FLAGS += -mabi=64
		ifeq ($(OCTEON), 1)
			COMMON_FLAGS += -mips64r2
		else
			COMMON_FLAGS += -mips64
		endif
	endif
endif

# ARM11 (Raspberri Pi I): the flag will enable HW unaligned access
ifeq ($(UNAME_M), armv6l)
	COMMON_FLAGS += -march=armv6
endif

CFLAGS += $(COMMON_FLAGS) -Isrc
CXXFLAGS += $(COMMON_FLAGS)
LDFLAGS += $(COMMON_FLAGS)

VPATH   = src:examples
SOURCES	= sdata.c sdbg.c senc.c sstring.c schar.c ssearch.c svector.c stree.c smap.c sdmap.c shash.c
HEADERS	= scommon.h $(SOURCES:.c=.h)
OBJECTS	= $(SOURCES:.c=.o)
LIBSRT	= libsrt.a
TEST	= stest
EXAMPLES = bench counter enc
EXES	= $(TEST) $(EXAMPLES)

# Rules for building: library, test, examples

all: $(EXES) run_tests
$(OBJECTS): $(HEADERS)
$(LIBSRT): $(OBJECTS)
	ar rcs $(LIBSRT) $(OBJECTS)
$(EXES): $% $(LIBSRT)

run_tests: stest
	@./$(TEST)
clean:
	@rm -f $(OBJECTS) $(LIBSRT) *\.o *\.gcno *\.gcda *\.out callgrind* out\.txt
	@for X in $(EXES) ; do rm -f $$X $$X.o ; done

