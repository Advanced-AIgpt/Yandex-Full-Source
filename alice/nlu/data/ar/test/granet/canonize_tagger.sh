#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

if [[ $# < 2 ]]; then
    echo "Usage: $0 BATCH FILTER [GRANET_OPTIONS]..."
    echo "Example: $0 small alarm_set"
    exit 1
fi

BATCH="$1"
FILTER="$2"
shift 2

GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"
GRAMMAR="$ARCADIA/alice/nlu/data/ar/granet/main.grnt"

if [[ ! -e "$GRANET" ]]; then
    "$ARCADIA/alice/nlu/data/ar/test/granet/prepare.sh"
fi

"$GRANET" batch canonize-tagger \
    -g "$GRAMMAR" \
    -b "$BATCH" \
    --filter "$FILTER" \
    "$@"
