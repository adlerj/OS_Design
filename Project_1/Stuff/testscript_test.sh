#!/bin/bash

file="128_128.txt"
name1="runtimes_wtc_proc_"$file
name2="runtimes_wtc_btproc_"$file
echo "WTC_PROC " $file >> "$name1"
./wtc_proc $file >> "$name1"
