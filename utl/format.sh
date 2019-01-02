#!/bin/bash
#
# format.sh
#
# libsrt code formatting
#
# Copyright (c) 2015-2019 F. Aragon. All rights reserved.
# Released under the BSD 3-Clause License (see the doc/LICENSE)
#

cp .clang-format ..
cd ..
find . -iname '*\.c' | while read file ; do
	clang-format-5.0 -i $file
done
rm .clang-format
