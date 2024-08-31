#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

if [[ $# < 2 ]]; then
    echo "Usage: $0 FORM TEXT"
    echo "Example: $0 alice.random_number 'Алиса, назови число от одного до двадцати трёх'"
    exit 1
fi

FORM="$1"
TEXT="$2"
shift 2

GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"
GRAMMAR="$ARCADIA/alice/nlu/data/en/granet/main.grnt"

if [[ ! -e "$GRANET" ]]; then
    "$ARCADIA/alice/nlu/data/en/test/granet/prepare.sh"
fi

"$GRANET" debug parser \
    -g "$GRAMMAR" \
    --form "$FORM" \
    --text "$TEXT" \
    "$@"
