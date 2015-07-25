#!/usr/bin/python3
#
# doc.py
#
# Automatic documentation generator.
#
# Example: doc/c2doc.py < src/sstring.h > output.html
#
# Tag syntax example from src/smap.h:
#	/* #API: |Insert into uint32-uint32 map|map; key; value|S_TRUE: OK, S_FALSE: insertion error|O(log n)|1;2| */
#	sbool_t sm_uu32_insert(sm_t **m, const suint32_t k, const suint32_t v);
#
#	/* #API: |a|b|c|d|e| */
#	return_type function_name(param1, ..., paramN);
#
#	a: function description
#	b: function parameters, separated by ";"
#	c: return value (optional, if none, put "-")
#	d: time and space complexity (space complexity being optional if no
#	   extra space), separated by ";"
#	e: implementation status, separated by ";"
#		#1, code coverage:
#			0 basic coverage (Coverity, clang analyzer)
#			1 covered by test (test + Valgrind)
#			2 covered by proof
#		#2, code quality:
#			0 not reviewed
#			1 reviewed: with pending problems
#			2 reviewed: clean (-Wall, style)
#			3 reviewed: clean, secure (security risk checked)
#			4 reviewed: clean, secure, state of the art (speed and space)
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
	fun_o = api_tokens[4].split(';') if len(api_tokens) > 4 else []
	fun_status = api_tokens[5].split(';') if len(api_tokens) > 5 else []
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
	doc.append(fun_o)
	doc.append(fun_status)
	return doc

def atoi(a) :
    try :
        return int(a)
    except ValueError:
        return 0

def fmt_coverage(c) :
	return	"[" + str(c) + "] " + \
		("basic (Coverity, clang analyzer)" if c == 0 else \
		"test covered (test + Valgrind)" if c == 1 else \
		"proof covered" if c == 2 else "?")

def fmt_quality(q) :
	return	"[" + str(q) + "] " + \
		("not reviewed" if q == 0 else \
		"reviewed, with quality issues" if q == 1 else \
		"reviewedi, clean (-Wall, style, speed)" if q == 2 else "?")


def fundoc2html( doc ) :
	fun_name = doc[0][0]
	fun_desc = doc[0][1]
	fun_params = doc[1][0]
	fun_params_desc = doc[1][1]
	fun_ret = doc[2][0].replace("static ", "")
	fun_ret_desc = doc[2][1]
	fun_o = doc[3][0] if len(doc) > 3 and len(doc[3]) >= 1 else ''
	fun_os = doc[3][1] if len(doc) > 3 and len(doc[3]) >= 2 else ''
	fun_coverage = atoi(doc[4][0]) if len(doc) > 4 and len(doc[4]) > 0 else 0
	fun_quality = atoi(doc[4][1]) if len(doc) > 4 and len(doc[4]) > 1 else 0
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
		params += '<li><b>Return</b> (<i>' + fun_ret.strip() + '</i>): ' + fun_ret_desc + '</li>'
	params += '<li><b>Time complexity:</b> ' + (fun_o.strip() if len(fun_o) > 0 else '(pending)') + '</li>'
	params += '<li><b>Space complexity:</b> ' + (fun_os.strip() if len(fun_os) > 0 else 'no extra space') + '</li>'
	params += '<li><b>Coverage:</b> ' + fmt_coverage(fun_coverage) + '</li>'
	params += '<li><b>Quality:</b> ' + fmt_quality(fun_quality) + '</li>'
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

