#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

if [[ -z "$1" ]]; then
    echo "Usage: $0 BATCH"
    echo "Example:"
    echo "  ./prepare.sh  # checkout data and build granet tool"
    echo "  $0 ut/medium"
    exit 1
fi

BATCH="$1"
GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"
GRAMMAR="$ARCADIA/alice/nlu/data/tr/granet/main.grnt"

shift

"$GRANET" \
    batch canonize \
    --missing \
    -g "$GRAMMAR" \
    -b "$BATCH" \
    "$@"
