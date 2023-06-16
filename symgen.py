#!/usr/bin/env python3
"""
	MIT License

	Copyright (c) 2023 Julian Scheffers

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
"""

import sys

symgen_ver="v1.0.0"

compiler = None
islib    = None
outtype  = None
infile   = None
outfile  = None

def showVersion():
	print("symgen.py {}".format(symgen_ver))

def showHelp():
	print("{} [-vhcSH] <impl(ementation)|lib(rary)> <infile.csv> <outfile> [compiler options]".format(sys.argv[0]))
	print()
	print("symgen.py - generate dynamic library with stub symbols")
	print("symgen.py is a tool intended to aid the development of KBELF built-in libraries.  "
		+ "It can be used to create the necessary symbol mapping tables for KBELF and "
		+ "the dynamic objects that serve as placeholders for the built-in libraries, "
		+ "both either in C source code and compiled object form.")
	print()
	print("Options:")
	print("    -v --version")
	print("        Print the version of this tool ({})".format(symgen_ver))
	print("    -h --help")
	print("        Show this help text.")
	print("    -c")
	print("        Create a C file instead of a compiled object.")
	print("    -S --assembly")
	print("        Create an assembly file instead of a compiled object.")
	print("    -H --header")
	print("        Create a header file instead of a a compiled object.")
	print("    --compiler=<gcc>")
	print("        Specify C compiler to use.")
	print("    - --")
	print("        End of options.")
	print()
	print("Input file format:")
	print("symgen.py will read CSV files and look for 5 columns:")
	print("    implementation - Symbol name for the implementer of the built-in library.")
	print("    symbol - Symbol name for the application.")
	print("    description - A comment to write at the definition of the entry.")
	print("    returns - The return type of the function.")
	print("    arguments - The arguments of the function.")
	print("The comparison is case-insensitive but there must be exactly one of each column and no other columns are allowed.")

def parseArgs(argv):
	global compiler, islib, outtype, infile, outfile, cflags
	compiler = None
	islib    = None
	outtype  = None
	infile   = None
	outfile  = None
	while len(argv) > 0:
		if argv[0] == '-' or argv[0] == '--':
			argv = argv[1:]
			break
		elif argv[0][0:2] == '--':
			arg = argv[0]
			val = None
			# Split at the '='.
			if '=' in arg:
				val = arg[arg.index('=')+1:]
				arg = arg[2:arg.index('=')]
			else:
				arg = arg[2:]
			if val and len(val) == 0:
				val = None
			# Check against THINGS.
			if arg == 'version':
				showVersion()
				exit(0)
			elif arg == 'help':
				showHelp()
				exit(0)
			elif arg == 'assembly':
				if val != None:
					print("Error: `--assembly` takes no value")
				if outtype != None:
					print("Error: Output type already specified ({})".format(outtype))
					exit(1)
				outtype = "assembly"
			elif arg == 'header':
				if val != None:
					print("Error: `--header` takes no value")
				if outtype != None:
					print("Error: Output type already specified ({})".format(outtype))
					exit(1)
				outtype = "header"
			elif arg == 'compiler':
				if val == None:
					print("Error: Expected an argument to `--compiler=`")
					exit(1)
				if compiler != None:
					print("Error: Compiler already specified ({})".format(compiler))
					exit(1)
				compiler = val
			else:
				print("Error: No such option `--{}`".format(arg))
			argv = argv[1:]
		elif argv[0][0] == '-':
			if 'h' in argv[0]:
				showHelp()
				exit(0)
			if 'v' in argv[0]:
				showVersion()
				exit(0)
			for char in argv[0][1:]:
				if char == 'c':
					if outtype != None:
						print("Error: Output type already specified ({})".format(outtype))
						exit(1)
					outtype = "c"
				elif char == 'S':
					if outtype != None:
						print("Error: Output type already specified ({})".format(outtype))
						exit(1)
					outtype = "assembly"
				elif char == 'H':
					if outtype != None:
						print("Error: Output type already specified ({})".format(outtype))
						exit(1)
					outtype = "header"
				else:
					print("Error: No such flag `-{}`".format(char))
			argv = argv[1:]
		else:
			break
	if len(argv) < 3:
		print("Error: Not enough arguments")
		exit(1)
	if argv[0] == 'impl' or argv[0] == 'implementation':
		islib = False
	elif argv[0] == 'lib' or argv[0] == 'library':
		islib = True
	else:
		print("Error: Expected 'impl' or 'lib', got '{}'".format(argv[0]))
	infile   = argv[1]
	outfile  = argv[2]
	compiler = compiler or "cc"
	cflags   = argv[3:]
