#
# Makefile for libsrt (POSIX systems: Linux, BSDs/Unix/Unix-like, etc.)
#
# Examples:
# Build with defaults: make
# Build with gcc and profiling: make CC=gcc PROFILING=1
# Build with gcc using C89/90 standard: make CC=gcc C99=0
# Build with g++ default C++ settings: make CC=g++
# Build with clang++ using C++11 standard: make CC=clang++ CPP11=1
# Build with TinyCC with debug symbols: make CC=tcc DEBUG=1
# Build with gcc cross compiler (PPC): make CC=powerpc-linux-gnu-gcc
# Build with gcc cross compiler (ARM): make CC=arm-linux-gnueabi-gcc
#
# Observations:
# - On FreeSD use gmake instead of make (as in that system "make" is "pmake", and
#   not GNU make). If not installed, use: pkg install gmake
#
# Copyright (c) 2015 F. Aragon. All rights reserved.
#

# Makefile parameters

ifndef C99
	C99 = 0
endif
ifndef C11
	C11 = 0
endif
ifndef CPP11
	CPP11 = 0
endif
ifndef CPP0X
	CPP0X = 0
endif
ifndef PROFILING
	PROFILING = 0
endif
ifndef DEBUG
	DEBUG = 0
endif
ifndef PEDANTIC
	PEDANTIC = 0
endif
ifndef MINIMAL_BUILD
	MINIMAL_BUILD = 0
endif
ifndef FORCE32
	FORCE32 = 0
endif
ifndef HAS_PNG
	HAS_PNG = 0
endif
ifndef HAS_JPG
	HAS_JPG = 0
endif

COMMON_FLAGS = -pipe

# Configure compiler context

ifndef UNAME
	UNAME = $(shell uname)
endif
ifndef UNAME_M
	UNAME_M = $(shell uname -m)
endif
ifndef OCTEON
	OCTEON = 0
	ifeq ($(UNAME_M), mips64)
		ifeq ($(UNAME), Linux)
			OCTEON = $(shell cat /proc/cpuinfo | grep cpu.model | grep Octeon | head -1 | wc -l)
		endif
	endif
endif

CPP_MODE = 0
GNUC = 0
CLANG = 0
ifeq ($(CC), gcc)
	GNUC = 1
endif
ifeq ($(CC), g++)
	GNUC = 1
	CPP_MODE = 1
endif
ifeq ($(CC), clang)
	CLANG = 1
endif
ifeq ($(CC), clang++)
	CLANG = 1
	CPP_MODE = 1
endif

ifeq ($(CC), tcc)
	PROFILING = 0
else
	ifeq ($(CPP11), 1)
		CPP_MODE = 1
	endif
	ifeq ($(CPP0X), 1)
		CPP_MODE = 1
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
		ifeq ($(GNUC), 1)
			CFLAGS += -Wall -Wextra
		endif
		ifeq ($(CLANG), 1)
			CFLAGS += -Weverything
		endif
		CFLAGS += -pedantic
	endif
	ifeq ($(GNUC), 1)
		CFLAGS += -Wstrict-aliasing
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

ifeq ($(FORCE32), 1)
	ifeq ($(UNAME_M), x86_64)
		COMMON_FLAGS += -m32
	endif
	ifeq ($(UNAME_M), mips64)
		ifeq ($(OCTEON), 1)
			COMMON_FLAGS += -march=octeon
		else
			COMMON_FLAGS += -mips32
		endif
	endif
else
	ifeq ($(UNAME_M), mips64)
		COMMON_FLAGS += -mabi=64
		ifeq ($(OCTEON), 1)
			COMMON_FLAGS += -march=octeon
		else
			COMMON_FLAGS += -mips64
		endif
	endif
endif

# ARM v6 little endian (e.g. ARM11 on Raspberr Pi I): the flag will enable HW unaligned access
ifeq ($(UNAME_M), armv6l)
	COMMON_FLAGS += -march=armv6
endif

# ARM v7-a little endian (e.g. ARM Cortex A5/7/8/9/15/17, QC Scorpion/Krait, etc.)
# (ARM v7-m and v7-r will be built as ARM v5)
ifeq ($(UNAME_M), armv7l)
	COMMON_FLAGS += -march=armv7-a
endif

ifeq ($(HAS_PNG), 1)
	COMMON_FLAGS += -DHAS_PNG
	LDFLAGS += -lz -lpng
endif

ifeq ($(HAS_JPG), 1)
	COMMON_FLAGS += -DHAS_JPG
	LDFLAGS += -ljpeg
endif

CFLAGS += $(COMMON_FLAGS) -Isrc $(EXTRA_CFLAGS)
LDFLAGS += $(COMMON_FLAGS)

VPATH   = src:examples
SOURCES	= sdata.c sdbg.c senc.c sstring.c schar.c ssearch.c svector.c stree.c smap.c sdmap.c shash.c
HEADERS	= scommon.h $(SOURCES:.c=.h) examples/*.h
OBJECTS	= $(SOURCES:.c=.o)
LIBSRT	= libsrt.a
TEST	= stest
EXAMPLES = bench counter enc table imgc
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
	@rm -f $(OBJECTS) $(LIBSRT) *\.o *\.dSYM *\.gcno *\.gcda *\.out callgrind* out\.txt
	@for X in $(EXES) ; do rm -f $$X $$X.o ; done

