#!/bin/bash

count=5
name1="runtimes_wtc_thr_"$1
name2="runtimes_wtc_btthr_"$1
echo "WTC_THR " $1 >> "$name1"
for ((c=1; c<=$count; c++)); do
	./wtc_thr $1 >> "$name1"
done

echo "WTC_BTTHR " $1 >> "$name2"
for ((c=1; c<=$count; c++)); do
	./wtc_btthr $1 >> "$name2"
done
