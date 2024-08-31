#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/*}"
ya() {
    "$ARCADIA/ya" "$@"
}

GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"
GRAMMAR="$ARCADIA/alice/nlu/data/ru/granet/main.grnt"

cd "$SCRIPT_DIR"
positive="${1}.pos.tsv"
negative="${1}.neg.tsv"

"$GRANET" dataset select      \
    -i $positive              \
    -n result/true_negative.tsv    \
    --lang ru                 \
    --grammar "$GRAMMAR"      \
    --keep-extra yes          \
    --form "$1"

"$GRANET" dataset select      \
    -i $negative              \
    -p result/false_positive.tsv    \
    --lang ru                 \
    --grammar "$GRAMMAR"      \
    --keep-extra yes          \
    --form "$1"

./result/output_results --true-negative result/true_negative.tsv --false-positive result/false_positive.tsv
