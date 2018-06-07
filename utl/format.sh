#!/bin/bash
cp .clang-format ..
cd ..
find . -iname '*\.c' | while read file ; do
	clang-format-5.0 -i $file
done
rm .clang-format
