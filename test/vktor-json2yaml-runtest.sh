#!/bin/bash

##
# vktor JSON pull-parser library
# 
# Copyright (c) 2009 Shahar Evron
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
##

# vktor test runner shell wrapper

CMD=./vktor-json2yaml
TEST=$1
OUTDIR=${OUTDIR-./}

# Find the tests' basename
TESTNAME=`basename $TEST .json`
TESTDIR=`dirname $TEST`

EXPECTED="$TESTDIR/$TESTNAME.exp"

# Check that we have everything

if [[ ! -x "$CMD" ]]; then
	echo "Test program $CMD is missing or is not executable, aborting" >&2
	exit 100
fi

if [[ ! -r "$TEST" ]]; then
	echo "Test script $TEST is missing or is not readable, aborting" >&2
	exit 101
fi

if [[ ! -r "$EXPECTED" ]]; then
	echo "Expected output file $EXPECTED is missing or is not readable, aborting" >&2
	exit 102
fi

if [[ ! -d "$OUTDIR" ]]; then
	echo "Provided output directory $OUTDIR is not a directory, aborting" >&2
	exit 103
fi

if [[ ! -w "$OUTDIR" ]]; then
	echo "Provided output directory $OUTPUT is not writable, aborting" >&2
	exit 104
fi

# Find diff
if [[ "x$DIFF" = "x" ]]; then
	DIFF=`which diff`
	if [[ "x$DIFF" = "x" ]]; then
		echo "The diff program was not found, aborting." >&2
		echo "Set the DIFF environment variable if you know where it is." >&2
		exit 105
	else 
		# Use unified diff format by default
		DIFF="$DIFF -u"
	fi
fi

# Run test, redirect stdout and stderr to files
$CMD < $TEST > "$OUTDIR/$TESTNAME-stdout.out" 2> "$OUTDIR/$TESTNAME-stderr.out"
RETVAL=$?

if [[ $RETVAL -ne 0 ]]; then
	echo "$TEST: program returned a non-zero status code" >&2
	exit $RETVAL
fi

# Compare output and expected using diff
$DIFF $EXPECTED "$OUTDIR/$TESTNAME-stdout.out" > "$OUTDIR/$TESTNAME.diff"
if [[ $? -ne 0 ]]; then
	echo "$TEST: test output differs from expected output" >&2
	exit 200
fi

# Clean up
rm -f $OUTDIR/$TESTNAME*.out ./$OUTDIR/$TESTNAME.diff 

# All is ok
exit 0
