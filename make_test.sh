#!/bin/bash
#
# make_test.sh
#
# Syntax: ./make_test.sh [mask]
#
# Where:
# - No parameters: all tests
# - mask = "or" operation of the following:
#		 1: Validate all available C/C++ builds
#		 2: Valgrind memcheck
#		 4: Clang static analyzer
#		 8: Generate documentation
#		16: Check coding style
#		32: Autoconf builds
#
# Examples:
# ./make_test.sh    # Equivalent to ./make_test.sh 31
# ./make_test.sh 1  # Do the builds
# ./make_test.sh 17 # Do the builds and check coding style
# ./make_test.sh 24 # Generate documentation and check coding style
#
# libsrt build, test, and documentation generation.
#
# Copyright (c) 2015-2019 F. Aragon. All rights reserved.
# Released under the BSD 3-Clause License (see the doc/LICENSE)
#

if (($# == 1)) && (($1 >= 1 && $1 < 64)) ; then TMUX=$1 ; else TMUX=63 ; fi
if [ "$SKIP_FORCE32" == "1" ] ; then
	FORCE32T=""
else
	FORCE32T="FORCE32=1"
fi

GCC=gcc
GPP=g++
CLANG=clang
CLANGPP=clang++
TCC=tcc

CLV=$(clang --version | grep version | awk '{print $3}' | \
      awk -F '-' '{print $1}')
for i in 7.0.0 ; do
	if [ "$i" = "$CLV" ] ; then
		CLANG=blacklisted_$CLANG ; CLANGPP=blacklisted_$CLANGPP
	fi
done

TEST_CC[0]="$GCC"
TEST_CC[1]="$GCC"
TEST_CC[2]="$GCC"
TEST_CC[3]="$GCC"
TEST_CC[4]="$CLANG"
TEST_CC[5]="$CLANG"
TEST_CC[6]="$CLANG"
TEST_CC[7]="$TCC"
TEST_CC[8]="$GPP"
TEST_CC[9]="$GPP"
TEST_CC[10]="$CLANGPP"
TEST_CC[11]="$CLANGPP"
TEST_CC[12]="arm-linux-gnueabi-$GCC"
TEST_CC[13]="arm-linux-gnueabi-$GCC"
TEST_CC[14]=${TEST_CC[0]}
TEST_CC[15]=${TEST_CC[0]}
TEST_CC[16]=${TEST_CC[0]}
TEST_CC[17]=${TEST_CC[0]}
TEST_CC[18]=${TEST_CC[0]}
TEST_CC[19]=${TEST_CC[0]}
TEST_CC[20]=${TEST_CC[0]}
TEST_CC[21]=${TEST_CC[0]}
TEST_CC[22]=${TEST_CC[0]}
TEST_CXX[0]="$GPP"
TEST_CXX[1]="$GPP"
TEST_CXX[2]="$GPP"
TEST_CXX[3]="$GPP"
TEST_CXX[4]="$CLANGPP"
TEST_CXX[5]="$CLANGPP"
TEST_CXX[6]="$CLANGPP"
TEST_CXX[7]="echo"
TEST_CXX[8]="$GPP"
TEST_CXX[9]="$GPP"
TEST_CXX[10]="$CLANGPP"
TEST_CXX[11]="$CLANGPP"
TEST_CXX[12]="arm-linux-gnueabi-$GPP"
TEST_CXX[13]="arm-linux-gnueabi-$GPP"
TEST_CXX[14]=${TEST_CXX[0]}
TEST_CXX[15]=${TEST_CXX[0]}
TEST_CXX[16]=${TEST_CXX[0]}
TEST_CXX[17]=${TEST_CXX[0]}
TEST_CXX[18]=${TEST_CXX[0]}
TEST_CXX[19]=${TEST_CXX[0]}
TEST_CXX[20]=${TEST_CXX[0]}
TEST_CXX[21]=${TEST_CXX[0]}
TEST_CXX[22]=${TEST_CXX[0]}
TEST_FLAGS[0]="C99=1 PEDANTIC=1"
TEST_FLAGS[1]="PROFILING=1"
TEST_FLAGS[2]="C99=0"
TEST_FLAGS[3]="C99=0 $FORCE32T"
TEST_FLAGS[4]="C99=1 PEDANTIC=1"
TEST_FLAGS[5]="C99=0 $FORCE32T"
TEST_FLAGS[6]="C99=1"
TEST_FLAGS[7]=""
TEST_FLAGS[8]=""
TEST_FLAGS[9]="CPP0X=1"
TEST_FLAGS[10]=""
TEST_FLAGS[11]="CPP11=1"
TEST_FLAGS[12]="C99=0"
TEST_FLAGS[13]="C99=1"
TEST_FLAGS[14]="${TEST_FLAGS[0]} ADD_FLAGS=-DS_DISABLE_SM_STRING_OPTIMIZATION"
TEST_FLAGS[15]="${TEST_FLAGS[0]} ADD_FLAGS=-DS_CRC32_SLC=0 \
		ADD_FLAGS2=-DS_DISABLE_SEARCH_GUARANTEE"
TEST_FLAGS[16]="${TEST_FLAGS[0]} ADD_FLAGS=-DS_CRC32_SLC=1"
TEST_FLAGS[17]="${TEST_FLAGS[0]} ADD_FLAGS=-DS_CRC32_SLC=4"
TEST_FLAGS[18]="${TEST_FLAGS[0]} ADD_FLAGS=-DS_CRC32_SLC=8"
TEST_FLAGS[19]="${TEST_FLAGS[0]} ADD_FLAGS=-DS_CRC32_SLC=12"
TEST_FLAGS[20]="${TEST_FLAGS[0]} ADD_FLAGS=-DS_CRC32_SLC=16"
TEST_FLAGS[21]="${TEST_FLAGS[0]} ADD_FLAGS=-DSD_DISABLE_HEURISTIC_GROWTH"
TEST_FLAGS[22]="${TEST_FLAGS[0]} ADD_FLAGS=-DS_DISABLE_LE_OPTIMIZATIONS"
TEST_DO_UT[0]="all"
TEST_DO_UT[1]="all"
TEST_DO_UT[2]="all"
TEST_DO_UT[3]="all"
TEST_DO_UT[4]="all"
TEST_DO_UT[5]="all"
TEST_DO_UT[6]="all"
TEST_DO_UT[7]="all"
TEST_DO_UT[8]="all"
TEST_DO_UT[9]="all"
TEST_DO_UT[10]="all"
TEST_DO_UT[11]="all"
TEST_DO_UT[12]="stest"
TEST_DO_UT[13]="stest"
TEST_DO_UT[14]=${TEST_DO_UT[0]}
TEST_DO_UT[15]=${TEST_DO_UT[0]}
TEST_DO_UT[16]=${TEST_DO_UT[0]}
TEST_DO_UT[17]=${TEST_DO_UT[0]}
TEST_DO_UT[18]=${TEST_DO_UT[0]}
TEST_DO_UT[19]=${TEST_DO_UT[0]}
TEST_DO_UT[20]=${TEST_DO_UT[0]}
TEST_DO_UT[21]=${TEST_DO_UT[0]}
TEST_DO_UT[22]=${TEST_DO_UT[0]}
ILOOP_FLAGS[0]=""
ILOOP_FLAGS[1]="DEBUG=1"
ILOOP_FLAGS[2]="MINIMAL=1"
ILOOP_FLAGS[3]="MINIMAL=1 DEBUG=1"
VALGRIND_LOOP_FLAGS[0]="DEBUG=1"
VALGRIND_LOOP_FLAGS[1]="DEBUG=1 ADD_FLAGS=-DS_DISABLE_SM_STRING_OPTIMIZATION"
VALGRIND_LOOP_FLAGS[2]="MINIMAL=1 DEBUG=1"
VALGRIND_LOOP_FLAGS[3]="MINIMAL=1 DEBUG=1"
VALGRIND_LOOP_FLAGS[3]+=" ADD_FLAGS=-DS_DISABLE_SM_STRING_OPTIMIZATION"
VALGRIND_LOOP_FLAGS[4]="DEBUG=1 ADD_FLAGS=-DS_FORCE_REALLOC_COPY"
ERRORS=0
NPROCS=0
MJOBS=1

if [ -e /proc/cpuinfo ] ; then # Linux CPU count
	NPROCS=$(grep processor /proc/cpuinfo | wc -l)
fi
if [ $(uname) = Darwin ] ; then # OSX CPU count
	NPROCS=$(sysctl hw.ncpu | awk '{print $2}')
fi
if (( NPROCS > MJOBS )) ; then MJOBS=$((2 * $NPROCS)) ; fi

echo "make_test.sh running..."

# Locate GNU Make
if [ "$MAKE" == "" ] ; then
	if type gmake >/dev/null 2>&1 ; then
		MAKE="gmake -f Makefile.posix"
	else
		MAKE="make -f Makefile.posix"
	fi
else
	MAKE="$MAKE -f Makefile.posix"
fi

for i in $GCC $CLANG ; do
	if type $i >/dev/null 2>&1 ; then $i --version ; fi ; done
if type $TCC >/dev/null 2>&1 ; then $TCC -version ; fi

if (($TMUX & 1)) ; then
	for ((i = 0; i < ${#TEST_CC[@]}; i++)) ; do
		if type ${TEST_CC[$i]} >/dev/null 2>&1 >/dev/null ; then
			for ((j = 0 ; j < ${#ILOOP_FLAGS[@]}; j++)) ; do
				CMD="$MAKE -j $MJOBS CC=${TEST_CC[$i]}"
				CMD+=" CXX=${TEST_CXX[$i]} ${TEST_FLAGS[$i]}"
				CMD+=" ${ILOOP_FLAGS[$j]} ${TEST_DO_UT[$i]}"
				$MAKE clean >/dev/null 2>&1
				echo -n "Test #$i.$j: [$CMD] ..."
				if $CMD >/dev/null 2>&1 ; then
					echo " OK"
				else 	echo " ERROR"
					ERRORS=$((ERRORS + 1))
				fi
			done
		else
			echo "Test #$i: ${TEST_CC[$i]} not found (skipped)"
		fi
	done
fi

VAL_ERR_TAG="ERROR SUMMARY:"
VAL_ERR_FILE=valgrind.errors

if (($TMUX & 2)) && type valgrind >/dev/null 2>&1 ; then
	for ((j = 0 ; j < ${#VALGRIND_LOOP_FLAGS[@]}; j++)) ; do
		MAKEFLAGS=${VALGRIND_LOOP_FLAGS[$j]}
		echo -n "Valgrind test ($MAKEFLAGS)..."
		VAL_ERR_FILEx=$VAL_ERR_FILE".$j"
		if $MAKE clean >/dev/null 2>&1 &&			\
		   $MAKE -j $MJOBS $MAKEFLAGS >/dev/null 2>&1 &&	\
		   valgrind --track-origins=yes --tool=memcheck		\
			    --leak-check=yes  --show-reachable=yes	\
			    --num-callers=20 --track-fds=yes		\
			    ./stest >/dev/null 2>$VAL_ERR_FILEx ; then
			VAL_ERRS=$(grep "$VAL_ERR_TAG" "$VAL_ERR_FILEx" | \
				   awk -F 'ERROR SUMMARY:' '{print $2}' | \
				   awk '{print $1}')
			if (( $VAL_ERRS > 0 )) ; then
				ERRORS=$((ERRORS + $VAL_ERRS))
				echo " ERROR"
			else
				echo " OK"
			fi
		else 	echo " ERROR"
			ERRORS=$((ERRORS + 1))
		fi
	done
fi

if (($TMUX & 4)) && type scan-build >/dev/null 2>&1 ; then
	echo -n "Clang static analyzer..."
	$MAKE clean
	if scan-build -v $MAKE CC=$CLANG 2>&1 >clang_analysis.txt ; then
		echo " OK"
	else	echo " ERROR"
		ERRORS=$((ERRORS + 1))
	fi
fi

if (($TMUX & 8)) ; then
	$MAKE clean
	OUT_DOC=doc/out
	mkdir $OUT_DOC 2>/dev/null
	if type python3 >/dev/null 2>&1 ; then
		echo "Documentation generation test..."
		if ! utl/mk_doc.sh src $OUT_DOC ; then
			ERRORS=$((ERRORS + 1))
		fi
	else
		echo "WARNING: doc not generated (python3 not found)"
	fi
	if type gcov >/dev/null 2>&1 ; then
		echo "Coverage report generation..."
		COVERAGE_OUT=$OUT_DOC/coverage.txt
		$MAKE -j $MJOBS CC=$GCC PROFILING=1 2>/dev/null >/dev/null
		for f in schar scommon sdata senc shash smap smset shmap \
			 shset ssearch ssort sstring sstringo stree svector \
			 stest ; do
			gcov $f.c >/dev/null 2>/dev/null
		done
		rm -f $COVERAGE_OUT 2>/dev/null
		ls -1 *c.gcov | awk -F '.gcov' '{print $1}' | while read f ; do
			echo "$f:"$(gcov $f | sed -e "/$f.gcov/"',$d' | \
			     awk -F ':' '{print $2}' | awk '{print $1}') >> \
								$COVERAGE_OUT
		done
		echo "$COVERAGE_OUT:"
		cat $COVERAGE_OUT
	else
		echo "WARNING: coverage report not generated (gcov not found)"
	fi
fi

if (($TMUX & 16)) ; then
	echo "Checking style..."
	SRCS="src/*c src/saux/*c test/*c test/*h Makefile.posix Makefile.am"
	ls -1  $SRCS *\.sh utl/*\.sh | while read line ; do
		if ! utl/check_style.sh "$line" ; then
			echo "$line... ERROR: style"
			ERRORS=$((ERRORS + 1))
		fi
	done
	if type egrep >/dev/null 2>&1 ; then
		find $SRCS src/*h src/saux/*h LICENSE README.md doc \
		     make_test.sh -type f | \
		while read line ; do
			if (($(sed -n '/ \+$/p' <$line | wc -l) > 0)) ; then
				echo "$line... ERROR, trailing spaces:"
				sed -n '/ \+$/p' <$line | while read l2 ; do
					echo -e "\t$l2[*]"
				done
				ERRORS=$((ERRORS + 1))
			fi
		done
	fi
fi

$MAKE clean

if (($TMUX & 32)) ; then
	echo "Autoconf build:"
	for j in ./bootstrap.sh ./configure "make -j $NPROCS" \
		 "make check" "make clean" ; do
		echo -n -e "\t$j: "
		if $j >/dev/null 2>&1 ; then
			echo "OK"
		else
			echo "ERROR"
			ERRORS=$((ERRORS + 1))
			break
		fi
	done
	make clean >/dev/null 2>&1
fi

exit $((ERRORS > 0 ? 1 : 0))
