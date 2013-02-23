#!/bin/bash

count=5

	for ((c=1; c<=$count; c++)); do
		./wtc_proc $file >> "$name1"
	done

        echo "WTC_BTPROC " $file >> "$name2"
        for ((c=1; c<=$count; c++)); do
                ./wtc_btproc $file >> "$name2"
        done
done
