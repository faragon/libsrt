#
# src/Makefile.am
#
# libsrt library Automake template
#
# Copyright (c) 2015-2019 F. Aragon. All rights reserved.
# Released under the BSD 3-Clause License (see the doc/LICENSE)
#

MAINTAINERCLEANFILES = Makefile.in
lib_LTLIBRARIES = libsrt.la
libsrt_la_SOURCES = sbitset.c shmap.c shset.c smap.c smset.c sstring.c \
		  svector.c saux/schar.c saux/scommon.c saux/sdata.c \
		  saux/sdbg.c saux/senc.c saux/shash.c saux/ssearch.c \
		  saux/ssort.c saux/sstringo.c saux/stree.c
library_include_HEADERS = libsrt.h sbitset.h shmap.h shset.h smap.h smset.h \
		  sstring.h svector.h saux/schar.h saux/sconfig.h \
		  saux/scrc32.h saux/sdbg.h saux/shash.h saux/ssort.h \
		  saux/stree.h saux/scommon.h saux/scopyright.h saux/sdata.h \
		  saux/senc.h saux/ssearch.h saux/sstringo.h
library_includedir = $(includedir)/libsrt
