#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

ya() {
    "$ARCADIA/ya" "$@"
}

DATA_DIR="$ARCADIA/alice/nlu/data/wizard_ru"

ya make -r --checkout \
  "$DATA_DIR/granet" \
  "$DATA_DIR/test/pool" \
  "$ARCADIA/alice/nlu/granet/tools/granet"
