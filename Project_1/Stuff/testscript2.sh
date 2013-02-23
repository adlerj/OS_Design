#!/bin/bash

count=10

for file in *.txt; do
	echo "WTC_THR " $file >> runtimes_wtc_thr_2.output
	for ((c=1; c<=$count; c++)); do
		./wtc_thr $file >> runtimes_wtc_thr_2.output
	done
done