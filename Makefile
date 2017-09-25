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
# Build without CRC32 hash tables, 1 bit/loop (100MB/s on i5@3GHz):
#	make ADD_CFLAGS="-DS_CRC32_SLC=0"
#   or	make MINIMAL=1
# Build with CRC32 1024 byte hash table, 1 byte/loop (400MB/s on i5@3GHz):
#	make ADD_CFLAGS="-DS_CRC32_SLC=1"
# Build with CRC32 4096 byte hash table, 4 bytes/loop (1000MB/s on i5@3GHz):
#	make ADD_CFLAGS="-DS_CRC32_SLC=4"
# Build with CRC32 8192 byte hash table, 8 bytes/loop (2000MB/s on i5@3GHz):
#	make ADD_CFLAGS="-DS_CRC32_SLC=8"
# Build with CRC32 12288 byte hash table, 12 bytes/loop (2500MB/s on i5@3GHz):
#	make ADD_CFLAGS="-DS_CRC32_SLC=12"
#   or	make -this is the default-
# Build with CRC32 16384 byte hash table, 16 bytes/loop (2700MB/s on i5@3GHz):
#	make ADD_CFLAGS="-DS_CRC32_SLC=16"
#
# Observations:
# - On FreeSD use gmake instead of make (as in that system "make" is "pmake",
#   and not GNU make). If not installed, use: pkg install gmake
#
# Copyright (c) 2015-2017 F. Aragon. All rights reserved.
#

include Makefile.inc

VPATH   = src:src/saux:examples
SOURCES	= sdata.c sdbg.c senc.c sstring.c schar.c ssearch.c ssort.c svector.c \
	  stree.c smap.c smset.c shash.c sbitio.c scommon.c
ESOURCES= imgtools.c
HEADERS	= scommon.h $(SOURCES:.c=.h) examples/*.h
OBJECTS	= $(SOURCES:.c=.o)
EOBJECTS= $(ESOURCES:.c=.o)
LIBSRT	= libsrt.a
ELIBSRT = elibsrt.a
TEST	= stest
EXAMPLES = counter enc table imgc imgd
ifeq (,$(findstring tcc,$(CC)))
	# Add CPP targets only if not using TCC
	EXAMPLES += bench
endif
EXES	= $(TEST) $(EXAMPLES)

# Rules for building: library, test, examples

all: $(EXES) run_tests
$(OBJECTS): $(HEADERS)
$(LIBSRT): $(OBJECTS)
	ar rcs $@ $^
$(ELIBSRT): $(EOBJECTS)
	ar rcs $@ $^
$(EXES): $% $(ELIBSRT) $(LIBSRT)

run_tests: stest
	@./$(TEST)
clean:
	@rm -f $(OBJECTS) $(LIBSRT) $(ELIBSRT) *\.o *\.dSYM *\.gcno *\.gcda *\.out \
	       callgrind* out\.txt clang_analysis.txt *\.errors*
	@for X in $(EXES) ; do rm -f $$X $$X.o ; done

