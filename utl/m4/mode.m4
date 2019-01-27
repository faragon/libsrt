#
# m4/mode.m4
#
# libsrt m4 build configuration macros
#
# Copyright (c) 2015-2019 F. Aragon. All rights reserved.
# Released under the BSD 3-Clause License (see the doc/LICENSE)
#

# Develoment (debug) / release mode switch

AC_MSG_CHECKING("Build mode selection")

AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug], [turn on debug mode [default=no]]),
	 , enable_debug=no)

AC_ARG_ENABLE(profiling,
	AS_HELP_STRING([--enable-profiling], [turn on profiling mode [default=no]]),
	 , enable_profiling=no)

AC_ARG_ENABLE(vargs,
	AS_HELP_STRING([--enable-vargs], [Enable variable-argument support [default=yes]]),
	 , enable_vargs=yes)

AC_ARG_ENABLE(crc32,
	AS_HELP_STRING([--enable-crc], [valid bytes per loop: 0, 1, 2, 4, 8, 12, 16 [default=12]]),
	 , enable_crc=12)

AC_ARG_ENABLE(stropt,
	AS_HELP_STRING([--enable-stropt], [map/set/hmap/set string optimization [default=yes]]),
	 , enable_stropt=yes)

AC_ARG_ENABLE(leopt,
	AS_HELP_STRING([--enable-leopt], [Allow Little Endian optimizations, if available [default=yes]]),
	 , enable_leopt=yes)

AC_ARG_ENABLE(egrowth,
	AS_HELP_STRING([--enable-egrowth], [Heuristic -geometric- resize grow [default=yes]]),
	 , enable_egrowth=yes)

AC_ARG_ENABLE(slowrealloc,
	AS_HELP_STRING([--enable-slowrealloc], [force realloc using memory copy [default=no]]),
	 , enable_slowrealloc=no)

if test "$enable_profiling" = "yes"; then
	CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage -g -pg"
	CXXFLAGS="$CXXFLAGS -fprofile-arcs -ftest-coverage -g -pg"
	LDFLAGS="$LDFLAGS -lgcov -coverage"
fi

if test "$enable_debug" = "yes"; then
	CFLAGS="$CFLAGS -g -O0"
	CXXFLAGS="$CXXFLAGS -g -O0"
	AC_DEFINE([DEBUG], [], [])
	AC_MSG_RESULT(yes)
else
	CFLAGS="$CFLAGS -O3"
	CXXFLAGS="$CXXFLAGS -O3"
	AC_DEFINE([NDEBUG], [], [])
	AC_MSG_RESULT(no)
fi

if test "$enable_vargs" == "no"; then
	CFLAGS="$CFLAGS -DS_NO_VARGS"
	CXXFLAGS="$CXXFLAGS -DS_NO_VARGS"
fi

if test "$enable_crc" != "12"; then
	CFLAGS="$CFLAGS -DS_CRC32_SLC=$enable_crc32"
	CXXFLAGS="$CXXFLAGS -DS_CRC32_SLC=$enable_crc32"
fi

if test "$enable_stropt" == "no"; then
	CFLAGS="$CFLAGS -DS_DISABLE_SM_STRING_OPTIMIZATION"
	CXXFLAGS="$CXXFLAGS -DS_DISABLE_SM_STRING_OPTIMIZATION"
fi

if test "$enable_leopt" == "no"; then
	CFLAGS="$CFLAGS -DS_DISABLE_LE_OPTIMIZATIONS"
	CXXFLAGS="$CXXFLAGS -DS_DISABLE_LE_OPTIMIZATIONS"
fi

if test "$enable_egrowth" == "no"; then
	CFLAGS="$CFLAGS -DSD_DISABLE_HEURISTIC_GROWTH"
	CXXFLAGS="$CXXFLAGS -DSD_DISABLE_HEURISTIC_GROWTH"
fi

if test "$enable_slowrealloc" == "yes"; then
	CFLAGS="$CFLAGS -DS_FORCE_REALLOC_COPY"
	CXXFLAGS="$CXXFLAGS -DS_FORCE_REALLOC_COPY"
fi

AM_CONDITIONAL(DEBUG, test "$enable_debug" = yes)
