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
	echo "Example: $0 ." >&2
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
INDEX="$DOC_PATH_OUT/libsrt.html"

cd ../src
echo '<!DOCTYPE html><html><meta http-equiv="Content-Type" content="text/html; charset=latin1"><title>libsrt</title><body>'\
     '<h1>libsrt documentation </h1><br><br>' > "$INDEX"
for i in *\.h ; do
	echo -n "$i: " >&2
	if $XPWD/c2doc.py "Documentation for $i" < "$i" >"$DOC_PATH_OUT/$i.html" ; then
		echo "OK" >&2
		if (( $(wc -l <"$DOC_PATH_OUT/$i.html") > 2 )) ; then
			echo '<br><a href="'"$i.html"'">'"$i"'</a><br>' >> "$INDEX"
		fi
	else	ERRORS=$((ERRORS + 1))
	fi
done
echo '</body></html>' >> "$INDEX"
cd "$XPWD"

exit $ERRORS

