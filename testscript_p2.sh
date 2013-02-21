#!/bin/bash

count=5

for file in *.txt; do
	name2="runtimes_wtc_btproc_"$file".output"
    echo "WTC_BTPROC " $file >> "$name2"
    for ((c=1; c<=$count; c++)); do
            ./wtc_btproc $file >> "$name2"
    done
done
