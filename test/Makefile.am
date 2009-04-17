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

# vktor test programs automake Makefile template

check_PROGRAMS = vktor-json2yaml

vktor_json2yaml_SOURCES = vktor-json2yaml.c

AM_CFLAGS = $(VKTOR_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/lib

LDADD = $(top_srcdir)/lib/libvktor.la

OUTDIR=results
TESTS_ENVIRONMENT = OUTDIR=$(OUTDIR) ./vktor-runtest.sh 
TESTS = tests/*

EXTRA_DIST = vktor-runtest.sh \
             $(TESTS)

clean-local:
	rm -rf $(OUTDIR)
