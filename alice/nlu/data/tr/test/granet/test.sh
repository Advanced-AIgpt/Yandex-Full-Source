#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

if [[ -z "$1" ]]; then
    echo "Usage: $0 BATCH [RESULT_NAME]"
    echo "Example:"
    echo "  ./prepare.sh  # checkout data and build granet tool"
    echo "  $0 small my_result"
    exit 1
fi

BATCH="$1"
RESULT_NAME="${2:-$(date '+%Y-%m-%d-%H%M%S')}"
GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"
GRAMMAR="$ARCADIA/alice/nlu/data/tr/granet/main.grnt"

shift
[[ $# == 0 ]] || shift

"$GRANET" \
    batch test \
    -g "$GRAMMAR" \
    -b "$BATCH" \
    -o "$BATCH/results/$RESULT_NAME" \
    "$@"
