#!/bin/bash
#
# mkdoc.sh
#
# Automatic documentation generator. *work in progress*
#
# Copyright (c) 2015 F. Aragon. All rights reserved.
#

trim()
{
	t=$1
	t=${t%% }
	echo "${t## }" | sed 's/\*/\\\*/g'
}

fv_parse()
{
	TSEP=' '
	if [ "$(echo $1 | grep '\\\*\\\*' 2>/dev/null | wc -l)" == "1" ] ; then
		TSEP='**'
	else
		if [ $(echo "$1" | grep '\\\*' 2>/dev/null | wc -l) == 1 ] ; then
			TSEP='*'
		fi
	fi
	FNAME=$(trim $(echo "$1" | awk -F "$TSEP" '{print $NF}'))
	FTYPE=$(trim $(echo "$1" | awk -F "$FNAME" '{print $1}'))
	echo "$FTYPE:$FNAME"
}

fv_type()
{
	echo "$1" | awk -F : '{print $1}'
}

fv_name()
{
	echo "$1" | awk -F : '{print $2}'
}

# Get metainformation and function prototype:
grep -Pzo "#API:[\s\S]*?\)[\n;]" ../*\.c | grep -v '^$' | sed 's/\*\///g' | sed 's/\/\*//g' | sed 's/\*/\\\\\*/g' | while read api ; do
	read proto
	PL=$(echo $proto | awk -F '(' '{print $1}')
	FPARAMS=$(echo $proto | awk -F '(' '{print $2}' | awk -F ')' '{print $1}')

	FNT=$(fv_parse "$PL")
	FNAME=$(fv_name "$FNT")
	FTYPE=$(fv_type "$FNT")
	echo "$FTYPE -- $FNAME -++- $PL"
	#echo "$FTYPE -- $FNAME -++- $FPARAMS"
	#echo "$api --- $prototype"

done

# TODO: generate HTML doc and man pages


