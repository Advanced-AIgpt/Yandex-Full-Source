#!/bin/bash
SEED=1234
if [ "$1" ]; then
    SEED=$1
fi
echo | awk -v seed=$SEED 'BEGIN {srand(seed);} {for (;;) printf "%c", int(256*rand());}'
