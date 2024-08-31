#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

# "$SCRIPT_DIR/../prepare.sh"

BATCH="$SCRIPT_DIR"
GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"
GRAMMAR="$ARCADIA/alice/nlu/data/wizard_ru/granet/main.grnt"

"$GRANET" \
    batch canonize \
    --missing \
    -g "$GRAMMAR" \
    -b "$BATCH" \
    "$@"
