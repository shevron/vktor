#!/bin/sh

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

TESTFILE=$1
OUTDIR=${OUTDIR-./}
CWD=$(pwd)

# check that the test file exists
if test ! -r "$TESTFILE"; then
	echo "ERR: test file '$TESTFILE' is not readable" >&2
	exit 200
fi

# check that the output directory exists
if test ! -d "$OUTDIR"; then
	# try to create output directory
	mkdir "$OUTDIR" 2> /dev/null
	if test $? -ne 0; then
		echo "ERR: output directory '$OUTDIR' could not be created" >&2
		exit 201
	fi
fi

if test ! -w "$OUTDIR"; then
	echo "ERR: output directory '$OUTDIR' is not writable" >&2
	exit 202
fi

# find diff
if test ! -x "$DIFF"; then
	DIFF=$(which diff)
	if test -z "$DIFF"; then
		echo "ERR: diff not found, set the DIFF env variable to diff location" >&2
		exit 203
	fi
fi
	
# set some default values
TEST_NAME=$(basename $TESTFILE)
TEST_STDIN=""
TEST_STDOUT=""
TEST_STDERR=""
TEST_RETVAL=0

SKIP_STDOUT=0
SKIP_STDERR=0
SKIP_RETVAL=0

# load the test file
source $TESTFILE

# make sure test program is executable
if test ! -x "$TEST_PROG"; then
	echo "ERR: test program '$TEST_PROG' is not executable" >&2
	exit 204
fi

# make sure test has a name
if test -z "$TEST_NAME"; then
	echo "ERR: test defined in $TESTFILE has no name" >&2
	exit 205
fi

# run the test
echo $TEST_STDIN > $OUTDIR/$TEST_NAME.stdin

$CWD/$TEST_PROG  < $OUTDIR/$TEST_NAME.stdin  \
                 > $OUTDIR/$TEST_NAME.stdout \
                2> $OUTDIR/$TEST_NAME.stderr
RETVAL=$?

# check return value
if test $SKIP_RETVAL -eq 0; then
	if test $TEST_RETVAL -ne $RETVAL; then
		echo "FAIL: retun value ($RETVAL) is not as expected ($TEST_RETVAL)" >&2
		exit 1
	fi
fi

# compare stdout
if test $SKIP_STDOUT -eq 0; then
	echo "$TEST_STDOUT" | $DIFF -u - $OUTDIR/$TEST_NAME.stdout > $OUTDIR/$TEST_NAME.stdout.diff
	if test $? -ne 0; then
		echo "FAIL: standard output does not match expected" >&2
		echo "      see $TEST_NAME.stdout.diff for details" >&2
		exit 1
	fi
fi

# compare stderr
if test $SKIP_STDERR -eq 0; then
	echo "$TEST_STDERR" | $DIFF -u - $OUTDIR/$TEST_NAME.stderr > $OUTDIR/$TEST_NAME.stderr.diff
	if test $? -ne 0; then
		echo "FAIL: standard error does not match expected" >&2
		echo "      see $TEST_NAME.stderr.diff for details" >&2
		exit 1
	fi
fi

# All is OK - clean up and exit 0
rm -f $OUTDIR/$TEST_NAME.std*
exit 0
