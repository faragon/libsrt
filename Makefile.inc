#
# Makefile.inc
#
# Common definitions for libsrt Makefiles
#
# Platforms: POSIX systems (Linux, BSDs/Unix/Unix-like, etc.)
#
# Copyright (c) 2015-2016 F. Aragon. All rights reserved.
#

# Makefile parameters

ifndef C99
	C99 = 0
endif
ifdef C89
	C90 = $(C89)
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
ifndef MINIMAL
	MINIMAL = 0
endif
ifdef MINIMAL_BUILD
	MINIMAL = $(MINIMAL_BUILD)
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
ifdef HAS_JPEG
	HAS_JPG = $(HAS_JPEG)
endif
ifndef HAS_LL1
	HAS_LL1 = 0
endif

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
			OCTEON = $(shell cat /proc/cpuinfo | grep cpu.model | \
					 grep Octeon | head -1 | wc -l)
		endif
	endif
endif

CFLAGS =
CXXFLAGS =
LDLIBS =
LDFLAGS =
COMMON_FLAGS = -pipe
CPP_MODE = 0
GNUC = 0
CLANG = 0

ifeq (,$(findstring mingw,$(CC)))
	ifeq (,$(findstring Darwin,$(UNAME)))
		# Add librt for all but for mingw and Darwin cases
		LDLIBS = -lrt
	endif
endif

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
				ifeq ($(C90), 1)
					CFLAGS += -std=c89
				endif
			endif
		endif
	endif

	ifeq ($(CPP11), 1)
		CXXFLAGS += -std=c++11
	endif
	ifeq ($(CPP0X), 1)
		CXXFLAGS += -std=c++0x
	endif

	ifeq ($(PEDANTIC), 1)
		ifeq ($(GNUC), 1)
			CFLAGS += -Wall -Wextra # -Werror
		endif
		ifeq ($(CLANG), 1)
			CFLAGS += -Weverything -Wno-old-style-cast \
				  -Wno-format-nonliteral
		endif
		COMMON_FLAGS += -pedantic
	endif
	ifeq ($(GNUC), 1)
		COMMON_FLAGS += -Wstrict-aliasing
	endif
endif
ifeq ($(DEBUG), 1)
	COMMON_FLAGS += -O0 -ggdb -DS_DEBUG
	ifeq ($(MINIMAL), 1)
		COMMON_FLAGS += -DS_MINIMAL
	endif
else
	ifeq ($(MINIMAL), 1)
		COMMON_FLAGS += -Os -DS_MINIMAL
	else
		COMMON_FLAGS += -O2
	endif
endif
ifeq ($(PROFILING), 1)
	COMMON_FLAGS += -ggdb
	# gcov flags:
	COMMON_FLAGS += -fprofile-arcs -ftest-coverage
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
	ifeq ($(UNAME_M), ppc64)
		COMMON_FLAGS += -m32
	endif
	ifeq ($(UNAME_M), mips64)
		ifeq ($(OCTEON), 1)
			GCC_COMMON_FLAGS += -march=octeon
		else
			GCC_COMMON_FLAGS += -mips32
		endif
	endif
else
	ifeq ($(UNAME_M), ppc64)
		COMMON_FLAGS += -m64
	endif
	ifeq ($(UNAME_M), mips64)
		GCC_COMMON_FLAGS += -mabi=64
		ifeq ($(OCTEON), 1)
			GCC_COMMON_FLAGS += -march=octeon
		else
			GCC_COMMON_FLAGS += -mips64
		endif
	endif
endif

# ARM v6 little endian (e.g. ARM11 on Raspberr Pi I): the flag will enable
# HW unaligned access
ifeq ($(UNAME_M), armv6l)
	COMMON_FLAGS += -march=armv6
endif

# ARM v7-a little endian (e.g. ARM Cortex A5/7/8/9/15/17, QC Scorpion/Krait)
# (ARM v7-m and v7-r will be built as ARM v5)
ifeq ($(UNAME_M), armv7l)
	COMMON_FLAGS += -march=armv7-a
endif

# ARM v8
ifeq ($(UNAME_M), armv8l)
	COMMON_FLAGS += -march=armv8-a
endif

ifneq ($(HAS_PNG), 0)
	COMMON_FLAGS += -DHAS_PNG=$(HAS_PNG)
	LDLIBS += -lz
	ifeq ($(HAS_PNG), 16)
		LDLIBS += -lpng16
	else
		ifeq ($(HAS_PNG), 15)
			LDLIBS += -lpng15
		else
			LDLIBS += -lpng
		endif
	endif
endif

ifeq ($(HAS_JPG), 1)
	COMMON_FLAGS += -DHAS_JPG
	LDLIBS += -ljpeg
endif

ifeq ($(HAS_LL1), 1)
	COMMON_FLAGS += -DHAS_LL1
endif

ifeq ($(GNUC), 1)
	COMMON_FLAGS += $(GCC_COMMON_FLAGS)
endif

CFLAGS += $(COMMON_FLAGS) -Isrc
CXXFLAGS += $(COMMON_FLAGS) -Isrc
LDFLAGS += $(COMMON_FLAGS)

ifdef ADD_CFLAGS
	CFLAGS += $(ADD_CFLAGS)
endif

ifdef ADD_CXXFLAGS
	CXXFLAGS += $(ADD_CXXFLAGS)
endif

ifdef ADD_FLAGS
	CFLAGS += $(ADD_FLAGS)
	CXXFLAGS += $(ADD_FLAGS)
endif

