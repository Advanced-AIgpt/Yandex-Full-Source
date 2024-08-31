#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

if [[ $# < 2 ]]; then
    echo "Usage: $0 ENTITY TEXT"
    echo "Example: $0 custom.food.mc_item_name 'Добавь биг мак комбо и две больших колы'"
    exit 1
fi

ENTITY="$1"
TEXT="$2"
shift 2

GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"
GRAMMAR="$ARCADIA/alice/nlu/data/ar/granet/main.grnt"

if [[ ! -e "$GRANET" ]]; then
    "$ARCADIA/alice/nlu/data/ar/test/granet/prepare.sh"
fi

"$GRANET" debug parser \
    -g "$GRAMMAR" \
    --entity "$ENTITY" \
    --text "$TEXT" \
    "$@"
