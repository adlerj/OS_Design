#!/bin/bash

count=10

for file in *.txt; do
	echo "WTC_THR " $file >> runtimes_wtc_thr.output
	for ((c=1; c<=$count; c++)); do
		./wtc_thr $file >> runtimes_wtc_thr.output
	done

for file in *.txt; do
	echo "WTC_BTTHR " $file >> runtimes_wtc_btthr.output
	for ((c=1; c<=$count; c++)); do
		./wtc_btthr $file >> runtimes_wtc_btthr.output
	done
done
done