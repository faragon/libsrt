#
# test/Makefile.am
#
# libsrt Automake template for tests and examples
#
# Copyright (c) 2015-2019 F. Aragon. All rights reserved.
# Released under the BSD 3-Clause License (see the doc/LICENSE)
#

MAINTAINERCLEANFILES = Makefile.in

check_PROGRAMS = stest
bin_PROGRAMS = bench counter counterpp enc histogram histogrampp imgc imgd
TESTS = $(check_PROGRAMS)

stest_SOURCES = stest.c
stest_LDADD = ../src/libsrt.la

bench_SOURCES = bench.cc
bench_LDADD = ../src/libsrt.la

counter_SOURCES = counter.c
counter_LDADD = ../src/libsrt.la

counterpp_SOURCES = counterpp.cc
counterpp_LDADD = ../src/libsrt.la

enc_SOURCES = enc.c
enc_LDADD = ../src/libsrt.la

histogram_SOURCES = histogram.c
histogram_LDADD = ../src/libsrt.la

histogrampp_SOURCES = histogrampp.cc
histogrampp_LDADD = ../src/libsrt.la

imgc_SOURCES = imgc.c imgtools.c
imgc_LDADD = ../src/libsrt.la

imgd_SOURCES = imgd.c imgtools.c
imgd_LDADD = ../src/libsrt.la
