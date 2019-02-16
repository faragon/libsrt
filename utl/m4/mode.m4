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

AC_ARG_ENABLE(onsearch,
	AS_HELP_STRING([--enable-onsearch], [O(n) string search complexity -disabling it you could get minor speed boost in most cases, and slowdowns in corner some corner cases- [default=yes]]),
	 , enable_onsearch=yes)

AC_ARG_ENABLE(leopt,
	AS_HELP_STRING([--enable-leopt], [Allow Little Endian optimizations, if available [default=yes]]),
	 , enable_leopt=yes)

AC_ARG_ENABLE(egrowth,
	AS_HELP_STRING([--enable-egrowth], [Heuristic -geometric- resize grow [default=yes]]),
	 , enable_egrowth=yes)

AC_ARG_ENABLE(slowrealloc,
	AS_HELP_STRING([--enable-slowrealloc], [force realloc using memory copy [default=no]]),
	 , enable_slowrealloc=no)

AC_ARG_WITH(libpng,
	    AS_HELP_STRING([--with-libpng], [enable libpng usage]),
	    [with_libpng=$withval],
	    [with_libpng='no'])

AC_ARG_WITH(libjpeg,
	    AS_HELP_STRING([--with-libjpeg], [enable libjpeg usage]),
	    [with_libjpeg=$withval],
	    [with_libjpeg='no'])

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

if test "$enable_onsearch" == "no"; then
	CFLAGS="$CFLAGS -DS_DISABLE_SEARCH_GUARANTEE"
	CXXFLAGS="$CXXFLAGS -DS_DISABLE_SEARCH_GUARANTEE"
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

have_libpng='no'
LIBPNG_LIBS=''
if test "$with_libpng" != 'no'; then
	AC_MSG_CHECKING(for LIBPNG support )
	AC_MSG_RESULT()
	failed=0
	passed=0
	AC_CHECK_HEADER([libpng/png.h],[passed=`expr $passed + 1`],[failed=`expr $failed + 1`])
	AC_CHECK_LIB([png],[png_write_image],[passed=`expr $passed + 1`],[failed=`expr $failed + 1`],)
	AC_MSG_CHECKING(if libpng png_write_image is available)
	if test $passed -gt 0; then
		if test $failed -gt 0; then
			AC_MSG_RESULT(no -- some components failed test)
			have_libpng='no (failed tests)'
		else
			LIBPNG_LIBS='-lpng'
			LIBS="$LIBPNG_LIBS $LIBS"
			AC_DEFINE(HAS_PNG,1,Define if you have libpng)
			AC_MSG_RESULT(yes)
			have_libpng='yes'
		fi
	else
		AC_MSG_RESULT(no)
	fi
fi

have_libjpeg='no'
LIBJPEG_LIBS=''
if test "$with_libjpeg" != 'no'; then
	AC_MSG_CHECKING(for LIBJPEG support )
	AC_MSG_RESULT()
	failed=0
	passed=0
	AC_CHECK_HEADER([jpeglib.h],[passed=`expr $passed + 1`],[failed=`expr $failed + 1`])
	AC_CHECK_LIB([jpeg],[jpeg_write_scanlines],[passed=`expr $passed + 1`],[failed=`expr $failed + 1`],)
	AC_MSG_CHECKING(if libjpeg jpeg_write_scanlines is available)
	if test $passed -gt 0; then
		if test $failed -gt 0; then
			AC_MSG_RESULT(no -- some components failed test)
			have_libjpeg='no (failed tests)'
		else
			LIBJPEG_LIBS='-ljpeg'
			LIBS="$LIBJPEG_LIBS $LIBS"
			AC_DEFINE(HAS_JPG,1,Define if you have libjpeg)
			AC_MSG_RESULT(yes)
			have_libjpeg='yes'
		fi
	else
		AC_MSG_RESULT(no)
	fi
fi

AM_CONDITIONAL(DEBUG, test "$enable_debug" = yes)
