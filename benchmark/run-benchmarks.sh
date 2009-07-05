#!/bin/sh

for FILE in $@; do
	./vktor-benchmark "$FILE"
	RET=$?
	if test $RET -ne 0; then
		echo "Error: benchmarking $FILE failed, return code is $RET" &1>2 
		exit
	fi
done

