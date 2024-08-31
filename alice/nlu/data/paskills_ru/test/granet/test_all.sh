#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

if [[ $# < 1 ]]; then
    echo "Usage: $0 BATCH [GRANET_OPTIONS]..."
    echo "Example:"
    exit 1
fi

BATCH="$1"
shift 1

GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"
GRAMMAR="$ARCADIA/alice/nlu/data/paskills_ru/granet/main.grnt"

if [[ ! -e "$GRANET" || ! -e "$ARCADIA/alice/nlu/data/ru/test/pool/alice3v2.tsv" ]]; then
    "$ARCADIA/alice/nlu/data/ru/test/granet/prepare.sh"
fi

"$GRANET" \
    batch test \
    -g "$GRAMMAR" \
    -b "$BATCH" \
    -o "$BATCH/results" \
    "$@"
