#!/bin/bash
threads=1
while [ $threads -le 100 ]; do
	# get the runtime in the format "XmY.YYYs" for the real time
	time=$( (time ./mdu /pkg -j$threads) 2>&1 | grep real | awk '{print $2}')
	# write the results to a file
	echo "$threads $time" >> "results11.txt"
	threads=$((threads + 1))
done