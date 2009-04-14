#!/bin/sh

GIT=$(which git)
if test -n "$GIT"; then
	$GIT log -1 $@ | head -n3
fi
