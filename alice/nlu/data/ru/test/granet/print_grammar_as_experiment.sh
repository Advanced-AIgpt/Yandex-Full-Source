#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"
BASE_DIR="$ARCADIA/alice/nlu/data/ru/granet"

GRAMMAR="$1"
[[ "$GRAMMAR" ]] || { GRAMMAR="$BASE_DIR/main.grnt"; }

if [ ! -x "$GRANET" ]; then
    $ARCADIA/ya make -r --checkout "$GRANET"
fi

echo -n 'bg_granet_source_text='
"$GRANET" grammar pack \
  --lang ru \
  --source-dir "$BASE_DIR" \
  -g "$GRAMMAR"
echo
