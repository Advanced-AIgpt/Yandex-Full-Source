#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
[[ "$ARCADIA" ]] || { ARCADIA="${SCRIPT_DIR%/alice/nlu*}"; }
ya() {
    "$ARCADIA/ya" "$@"
}

DATA_DIR="$ARCADIA/alice/nlu/data/kk"

ya make -r --checkout \
  "$DATA_DIR/granet" \
  "$DATA_DIR/test/pool" \
  "$DATA_DIR/test/granet/small" \
  "$ARCADIA/alice/nlu/granet/tools/granet"
