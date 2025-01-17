#!/bin/bash

convert_time_to_seconds() {
    local time_string=$1
    local minutes=$(echo $time_string | cut -d'm' -f1)
    local seconds=$(echo $time_string | cut -d'm' -f2 | cut -d's' -f1)
    echo $(bc <<< "$minutes*60 + $seconds")
}

threads=1
while [ $threads -le 100 ]; do
    total_time=0
    for i in {1..5}; do
        # Get the runtime in the format "XmY.YYYs" and convert it to seconds.
        runtime=$( (time ./mdu /pkg -j$threads) 2>&1 | grep real | awk '{print $2}')
        total_time=$(bc <<< "$total_time + $(convert_time_to_seconds $runtime)")
    done

    # Calculate the average time.
    average_time=$(bc <<< "scale=3; $total_time/5.0")

    # Write the results to a file.
    echo "$threads $average_time" >> "results111.txt"
    threads=$((threads + 1))
done