#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

ya() {
    "$ARCADIA/ya" "$@"
}

ya make -r --checkout "$ARCADIA/alice/nlu/granet/tools/vins/converter"
ya make -j0 --checkout "$ARCADIA/alice/vins"

$ARCADIA/alice/nlu/granet/tools/vins/converter/converter \
    --input $ARCADIA/alice/vins \
    --output $ARCADIA/alice/nlu/granet/tools/vins/converted \
    --debug
