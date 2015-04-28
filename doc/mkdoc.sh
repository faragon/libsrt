#!/bin/bash
#
# mkdoc.sh
#
# Automatic documentation generator. *work in progress*
#
# Copyright (c) 2015 F. Aragon. All rights reserved.
#

# Get metainformation and function prototype:
grep -Pzo "#API:[\s\S]*?\)[\n;]" ../*\.c | grep -v '^$'

# TODO: generate HTML doc and man pages


