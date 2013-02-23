#!/bin/bash

count=5

for file in *.txt; do
	name1="runtimes_wtc_proc_"$file".outptut"
	echo "WTC_PROC " $file >> "$name1"
	for ((c=1; c<=$count; c++)); do
		./wtc_proc $file >> "$name1"
	done
done
