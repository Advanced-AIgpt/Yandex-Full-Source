#!/bin/bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

function get-arcadia-root {
    local R="$DIR"
    while [[ ! -e "${R}/.arcadia.root" ]]; do
        if [[ "${R}" == "/" ]]; then
            echo "$0: must be run from inside Arcadia checkout" >&2
            exit  1
        fi
        R="$(dirname "${R}")"
    done
    echo "$R"
}

ARCADIA_ROOT="$(get-arcadia-root)"
YA_BIN=${YA_BIN:-$ARCADIA_ROOT/ya}

SCENARIOS=""
NL=$'\n'
for i in "$@"
do
    SCENARIOS="${SCENARIOS}  alice/kronstadt/scenarios/$i${NL}"
done

echo "PEERDIR(${NL}${SCENARIOS})" > "$DIR/scenarios.inc"
KRONSTADT_SCENARIO_INC="${DIR#$ARCADIA_ROOT/}/scenarios.inc"

time $YA_BIN make --show-timings --stat -r -DKRONSTADT_SCENARIO_INC=${KRONSTADT_SCENARIO_INC} "$ARCADIA_ROOT/alice/kronstadt/tools/graph_generator"
$ARCADIA_ROOT/alice/kronstadt/tools/graph_generator/run.sh ru.yandex.alice.kronstadt.generator.GeneratorMainKt -a $ARCADIA_ROOT

