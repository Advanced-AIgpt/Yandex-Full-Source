#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

if [[ $# < 1 ]]; then
    echo "Usage: $0 TEXT"
    echo "Example: $0 'Алиса, включи номер один'"
    exit 1
fi

TEXT="$1"
shift 1

GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"

if [[ ! -e "$GRANET" ]]; then
    "$ARCADIA/alice/nlu/data/ar/test/granet/prepare.sh"
fi

"$GRANET" debug sample \
    --text "$TEXT" \
    "$@"
