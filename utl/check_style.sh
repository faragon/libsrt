#!/bin/bash
#
# check_style.sh
#
# Simple C code style check
# - Look for lines larger than 80 characters
#
# Copyright (c) 2015-2019 F. Aragon. All rights reserved.
# Released under the BSD 3-Clause License (see the doc/LICENSE)
#

if (( $(cat $* | expand | grep '.\{81\}' | wc -l) > 0 )) ; then
	cat $* | expand | grep '.\{81\}' | while read line ; do
		echo -e -n "\t" ; echo "$line"
	done
	echo -n "[over 80 characters] "
	exit 1
fi

if (( $(grep 'switch (' $* | grep -v { | wc -l) > 0 )) ; then
	grep 'switch (' $* | grep -v { | while read line ; do
		echo -e -n "\t" ; echo "$line"
	done
	echo -n "[switch without { in same line] "
	exit 2
fi

if (( $(cat $* | wc -l) != $(cat $* | sed '/^$/N;/^\n$/D' | wc -l) )) ; then
	echo -n "[consecutive blank lines] "
	exit 2
fi

exit 0 # ok

