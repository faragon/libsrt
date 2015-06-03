#!/bin/bash
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
TEST_FLAGS[0]=""
TEST_FLAGS[1]="PROFILING=1"
TEST_FLAGS[2]="C99=0"
TEST_FLAGS[3]="C99=0 FORCE32=1"
TEST_FLAGS[4]="C99=0"
TEST_FLAGS[5]="C99=0 FORCE32=1"
TEST_FLAGS[6]="C99=1"
TEST_FLAGS[7]=""
TEST_FLAGS[8]=""
TEST_FLAGS[9]="CPP0X=1"
TEST_FLAGS[10]=""
TEST_FLAGS[11]="CPP11=1"
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
			CMD="make CC=${TEST_CC[$i]} ${TEST_FLAGS[$i]} ${INNER_LOOP_FLAGS[$j]}"
			make clean >/dev/null 2>/dev/null
			echo -n "Test #$i.$j: [$CMD] ..." | tee -a $LOG
			if $CMD >>$LOG 2>&1
			then
				echo " OK" | tee -a $LOG
			else
				echo " ERROR" | tee -a $LOG
				ERRORS=1
			fi
		done
	else
		echo "Test #$i: ${TEST_CC[$i]} compiler not found (test skipped)" | tee -a $LOG
	fi
done

exit $ERRORS


