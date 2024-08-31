#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"
ya() {
    "$ARCADIA/ya" "$@"
}

GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"

ya make -j0 --checkout "$ARCADIA/alice/nlu/data/ru/granet"
ya make -r --checkout "$ARCADIA/alice/nlu/granet/tools/granet"

cd "$SCRIPT_DIR"
mkdir -p result

"$GRANET" dataset create \
    -i dataset.tsv \
    -o result/prepared.tsv \
    --lang ru
