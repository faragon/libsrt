#!/bin/bash
#
# check_style.sh
#
# Simple C code style check
# - Look for lines larger than 80 characters
#
# Copyright (c) 2015-2016 F. Aragon. All rights reserved.
#

if (( $(grep  '.\{81,\}' $* | wc -l) > 0 )) ; then
	echo -n "[over 80 characters] "
	exit 1
fi

if (( $(grep 'switch (' $* | grep -v { | wc -l) > 0 )) ; then
	echo -n "[switch without { in same line] "
	exit 2
fi

if (( $(cat $* | wc -l) != $(cat $* | sed '/^$/N;/^\n$/D' | wc -l) )) ; then
	echo -n "[consecutive blank lines] "
	exit 2
fi

exit 0 # ok

