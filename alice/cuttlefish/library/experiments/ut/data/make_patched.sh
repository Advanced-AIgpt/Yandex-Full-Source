#!/bin/bash

ARCADIA_ROOT="../../../../../../"
EXPERIMENTS_APP="$ARCADIA_ROOT/alice/uniproxy/library/experiments/app"

if [[ ! -x $EXPERIMENTS_APP/app ]]; then
    echo "Build experiments app..." >&2
    ya make $EXPERIMENTS_APP
fi

rm -f ./patched_events.jsons
while read SS; do
    echo "Run ..." >&2
    echo $SS | cat - ./sample_events.jsons | $EXPERIMENTS_APP/app \
        -e ./experiments.json \
        -m ./macros.json \
        1>>./patched_events.jsons
done < ./synchronize_states.jsons


