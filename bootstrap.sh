#!/bin/sh
#
# bootstrap.sh
#
# libsrt 'configure' bootstrap
#
# Copyright (c) 2015-2019 F. Aragon. All rights reserved.
# Released under the BSD 3-Clause License (see the doc/LICENSE)
#
ARC="autoreconf -f -i"
if ! $ARC >/dev/null 2>&1 ; then
	echo "Bootstrap error: $ARC failed (code: $?)" >&2
fi
