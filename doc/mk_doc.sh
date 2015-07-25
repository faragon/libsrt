#!/bin/bash
#
# mk_doc.sh
#
# libsrt documentation generation
#
# Copyright (c) 2015 F. Aragon. All rights reserved.
#

if [ $# != 1 ] ; then
	echo "Syntax: $0 out_path" >&2
	exit 1
fi

DOC_PATH_OUT=$1
ERRORS=0

if [ ! -e "$DOC_PATH_OUT" ] ; then
	echo "Path $1 not found" >&2
	exit 2
fi

DOC_PATH_OUT=$(realpath $DOC_PATH_OUT)

XPWD=$PWD
cd ../src
for i in *\.h ; do
	echo -n "$i: " >&2
	if $XPWD/c2doc.py "Documentation for $i" < "$i" >"$DOC_PATH_OUT/$i.html" ; then
		echo "OK" >&2
	else	ERRORS=$((ERRORS + 1))
	fi
done
cd "$XPWD"

exit $ERRORS

