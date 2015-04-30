#!/usr/bin/python3
#
# doc.py
#
# Automatic documentation generator. *work in progress*
#
# Example: ./c2doc.py <../sstring.c
#
# Copyright (c) 2015 F. Aragon. All rights reserved.
#

import sys

# functions

def mkdoc( apidoc, proto ) :
	api_tokens = apidoc.split('|')
	fun_desc = api_tokens[1]
	fun_params_desc = api_tokens[2].split(';')
	fun_ret_desc = api_tokens[3]
	fun_o = api_tokens[4]
	tmp = proto.split('(');
	split_char = '*' if tmp[0].find('*') >= 0 else ' '
	tmp2 = tmp[0].split(split_char)
	fun_name = tmp2[len(tmp2) - 1]
	fun_ret = tmp[0].split(fun_name)[0].strip()
	fun_params = tmp[1].split(')')[0].split(';')

	doc = []
	doc.append([fun_name, fun_desc])
	doc.append([fun_params, fun_params_desc])
	doc.append([fun_ret, fun_ret_desc])
	doc.append([fun_o])

	return doc

def doc2html( doc ) :
	# TODO
	return str(doc)




# main

lines = sys.stdin.readlines()
i = 0
num_lines = len(lines) - 1
doc = []

while i < num_lines :
	api = lines[i].replace('\n', '')
	if api.find('#API:') < 0 :
		i += 1
		continue
	i += 1
	if i == num_lines :
		break
	while i < num_lines :
		fun = lines[i].replace('\n', '')
		if fun.find('#API') >= 0 :
			fun = ''
			break
		i += 1
		if fun.find('(') >= 0 :
			break
	if (len(fun) > 0 ) :
		doc.append(mkdoc(api, fun))

i = 0
for i in range(0, len(doc) - 1) :
	print( doc2html( doc[i] ) )


