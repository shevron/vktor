# Makefile.am for vktor library

VKTOR_CFLAGS = 

if ENABLE_DEBUG
VKTOR_CFLAGS += -Wall -g
endif

vktordir = $(includedir)/vktor
vktor_HEADERS = vktor.h
lib_LTLIBRARIES = libvktor.la
libvktor_la_SOURCES = vktor.c

AM_CFLAGS = $(DEPOS_CFLAGS) \
            $(VKTOR_CFLAGS)
