#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
[[ "$ARCADIA" ]] || { ARCADIA="${SCRIPT_DIR%/alice/nlu/granet*}"; }

if [[ $# < 1 ]]; then
    echo "Usage: $0 FILTER [GRANET_OPTIONS]..."
    echo "Example:"
    echo "  $0 show_traffic"
    exit 1
fi

FILTER="$1"
BATCH="$SCRIPT_DIR"
GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"

shift 1

"$GRANET" \
    batch update-entities \
    -b "$BATCH" \
    --filter "$FILTER" \
    "$@"
