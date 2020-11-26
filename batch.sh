#!/usr/bin/bash

DEFAULT_ITERATIONS=20

if [ $2 ]; then
    iterations=$2
else 
    iterations=$DEFAULT_ITERATIONS;
fi

all_perf=()
for i in $(seq 1 $iterations); 
do 
    perf=$(./test-$1 2>&1 | grep ratio | cut -c 33-40)
    echo "$i: $perf"
    all_perf+=($perf)
done

python3 stats.py ${all_perf[@]}