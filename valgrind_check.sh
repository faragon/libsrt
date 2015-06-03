make clean; make DEBUG=1 PROFILING=1 ; valgrind --track-origins=yes --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./stest
