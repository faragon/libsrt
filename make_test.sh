#!/bin/bash
if [ "$SKIP_FORCE32" == "1" ] ; then FORCE32T="" ; else FORCE32T="FORCE32=1" ; fi
TEST_CC[0]="gcc"
TEST_CC[1]="gcc"
TEST_CC[2]="gcc"
TEST_CC[3]="gcc"
TEST_CC[4]="clang"
TEST_CC[5]="clang"
TEST_CC[6]="clang"
TEST_CC[7]="tcc"
TEST_CC[8]="g++"
TEST_CC[9]="g++"
TEST_CC[10]="clang++"
TEST_CC[11]="clang++"
TEST_CC[12]="arm-linux-gnueabi-gcc"
TEST_CC[13]="arm-linux-gnueabi-gcc"
TEST_FLAGS[0]=""
TEST_FLAGS[1]="PROFILING=1"
TEST_FLAGS[2]="C99=0"
TEST_FLAGS[3]="C99=0 $FORCE32T"
TEST_FLAGS[4]="C99=0"
TEST_FLAGS[5]="C99=0 $FORCE32T"
TEST_FLAGS[6]="C99=1"
TEST_FLAGS[7]=""
TEST_FLAGS[8]=""
TEST_FLAGS[9]="CPP0X=1"
TEST_FLAGS[10]=""
TEST_FLAGS[11]="CPP11=1"
TEST_FLAGS[12]="C99=0"
TEST_FLAGS[13]="C99=1"
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
INNER_LOOP_FLAGS[0]=""
INNER_LOOP_FLAGS[1]="DEBUG=1"
INNER_LOOP_FLAGS[2]="MINIMAL_BUILD=1"
INNER_LOOP_FLAGS[3]="MINIMAL_BUILD=1 DEBUG=1"
LOG=out.txt
ERRORS=0
echo "make_test.sh start" > $LOG

for ((i = 0 ; i < ${#TEST_CC[@]}; i++))
do
	if type ${TEST_CC[$i]} >/dev/null 2>/dev/null
	then
		for ((j = 0 ; j < ${#INNER_LOOP_FLAGS[@]}; j++))
		do
			CMD="make CC=${TEST_CC[$i]} ${TEST_FLAGS[$i]} ${INNER_LOOP_FLAGS[$j]} ${TEST_DO_UT[$i]}"
			make clean >/dev/null 2>/dev/null
			echo -n "Test #$i.$j: [$CMD] ..." | tee -a $LOG
			if $CMD >>$LOG 2>&1 ; then
				echo " OK" | tee -a $LOG
			else 	echo " ERROR" | tee -a $LOG
				ERRORS=$((ERRORS + 1))
			fi
		done
	else
		echo "Test #$i: ${TEST_CC[$i]} compiler not found (test skipped)" | tee -a $LOG
	fi
done

if type valgrind >/dev/null 2>/dev/null
then
	echo -n "Valgrind test..." | tee -a $LOG
	if make clean >/dev/null 2>/dev/null ; make DEBUG=1 >>$LOG 2>&1 ; valgrind --track-origins=yes --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./stest >>$LOG 2>&1 ; then
		echo " OK" | tee -a $LOG
	else 	echo " ERROR" | tee -a $LOG
		ERRORS=$((ERRORS + 1))
	fi
fi

if type scan-build >/dev/null 2>/dev/null
then
	echo -n "Clang static analyzer..." | tee -a $LOG
	make clean
	if scan-build make CC=clang 2>&1 >clang_analysis.txt ; then
		echo " OK" | tee -a $LOG
	else	echo " ERROR" | tee -a $LOG
		ERRORS=$((ERRORS + 1))
	fi
fi

if type python3 >/dev/null 2>/dev/null
then
	echo "Documentation test..." | tee -a $LOG
	DOC_OUT=doc_out/
	mkdir $DOC_OUT 2>/dev/null
	cd src
	for i in $(ls *\.c) sbitset.h ; do
		echo -n "$i: " >&2
		if ../doc/c2doc.py "Documentation for $i" < "$i" >"../$DOC_OUT/$i.html" ; then
			echo "OK" >&2 | tee -a $LOG
		else	ERRORS=$((ERRORS + 1))
		fi
	done
	cd ..
fi

exit $ERRORS

