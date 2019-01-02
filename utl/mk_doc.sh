#!/bin/bash
#
# mk_doc.sh
#
# libsrt documentation generation
#
# Copyright (c) 2015-2019 F. Aragon. All rights reserved.
# Released under the BSD 3-Clause License (see the doc/LICENSE)
#

if [ $# != 2 ] ; then
	echo "Syntax: $0 src_path out_path" >&2
	echo "Example: $0 src out_doc" >&2
	exit 1
fi

SRC_PATH=$1
DOC_PATH_OUT=$2
ERRORS=0
mkdir "$DOC_PATH_OUT" 2>/dev/null

if [ ! -e "$DOC_PATH_OUT" ] ; then
	echo "Path $1 not found" >&2
	exit 2
fi

INDEX="$DOC_PATH_OUT/libsrt.html"

C2DOC_PY=$(echo $0 | sed 's/mk_doc.sh/c2doc.py/g')

if [ "$C2DOC_PY" == "$0" ] ; then
	echo "Error: this file name must be 'mk_doc.sh'"
	exit 1
fi

echo "<!DOCTYPE html><html><meta http-equiv=\"Content-Type\" content=\"text/"\
     "html; charset=latin1\"><title>libsrt</title><body><h3><a"\
     "href="https://github.com/faragon/libsrt">libsrt</a> documentation<br>" \
     "<br>" > "$INDEX"
README_PATH=README
if [ ! -f "$README_PATH"* ] ; then
	if [ -f "../$README_PATH"* ] ; then
		README_PATH="../$README_PATH"
	else
		if [ -f "../../$README_PATH"* ] ; then
			README_PATH="../../$README_PATH"
		fi
	fi
fi
grep "is a C library" "$README_PATH"* >> "$INDEX"

echo "<br><br>" >> "$INDEX"
for i in $SRC_PATH/*\.h ; do
	if ! grep '#API' "$i" >/dev/null 2>/dev/null ; then
		continue
	fi
	II=$(echo $i | sed 's/\/\//\//g')
	TGT=$(echo $i | awk -F '/' '{print $NF}')
	echo -n "$II: " >&2
	TGT_HTML="$DOC_PATH_OUT/$TGT.html"
	INC=$i
	if echo "$INC" | grep '/' 2>/dev/null >/dev/null ; then
		INC=$(echo "$INC" | awk -F '/' '{print $NF}')
	fi
	if $C2DOC_PY "$INC" < "$i" >"$TGT_HTML"
	then
		echo "OK" >&2
		if (( $(wc -l <"$TGT_HTML") > 2 )) ; then
			DESC=$(grep '#SHORTDOC' "$i" | \
			       awk -F '#SHORTDOC ' '{print $2}' | head -1)
			if [ "$DESC" = "" ] ; then DESC=$TGT ; fi
			if [ ! "$INC" = "" ] ; then INC="$INC: " ; fi
			echo '<br>'"$INC"'<a href="'"$TGT.html"'">'"$DESC"\
'</a><br>' >> "$INDEX"
		fi
	else	ERRORS=$((ERRORS + 1))
	fi
done
echo '</table></body></html>' >> "$INDEX"

exit $ERRORS

