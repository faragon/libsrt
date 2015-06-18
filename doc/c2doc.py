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
	fun_ret = tmp[0].split(fun_name)[0] #.strip()
	fun_params = tmp[1].split(')')[0].split(',')
	if len(fun_params) != len(fun_params_desc) :
		sys.stderr.write('Syntax error: "' + fun_name + \
			' incomplete parameters (number of arguments ' + \
			'in prototype and description mismatch)\n')
		sys.exit(1)
	doc = []
	doc.append([fun_name, fun_desc])
	doc.append([fun_params, fun_params_desc])
	doc.append([fun_ret, fun_ret_desc])
	doc.append([fun_o])
	return doc

def fundoc2html( doc ) :
	fun_name = doc[0][0]
	fun_desc = doc[0][1]
	fun_params = doc[1][0]
	fun_params_desc = doc[1][1]
	fun_ret = doc[2][0].replace("static ", "")
	fun_ret_desc = doc[2][1]
	proto = '<i>' + fun_ret + '</i><b>' + fun_name + '</b>('
	i = 0
	while i < len(fun_params) :
		proto += '<i>' + fun_params[i].strip() + '</i>'
		if i != len(fun_params) - 1 :
			proto += ', '
		i += 1
	proto += ')<br><br>'
	params = ''
	nfparams = len(fun_params)
	i = 0
	params += '<ul>'
	params += '<li>' + fun_desc + '</li>'
	while i < len(fun_params) :
		params += '<li><b>' + fun_params[i].strip() + '</b>: ' + \
			  fun_params_desc[i] + '</li>'
		i += 1
	if len(fun_ret_desc) :
		params += '<li><b><b>Return</b> (<i>' + fun_ret.strip() + '</i>): ' + fun_ret_desc + '</li>'
	params += '</ul><br>'
	return  proto + params + '<br>'

def doc2html( doc, title ) :
	i = 0
	out = '<!doctype html><html><body><h1>' + title + '</h1><br>';
	for i in range(0, len(doc) - 1) :
		out += fundoc2html( doc[i] ) + '\n'
	out += '</body></html>\n'
	return out

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

title = sys.argv[1] if len(sys.argv) > 1 else ''

print ( doc2html( doc, title ) )

