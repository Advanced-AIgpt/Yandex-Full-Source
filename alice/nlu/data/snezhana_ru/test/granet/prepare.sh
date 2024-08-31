#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

ya() {
    "$ARCADIA/ya" "$@"
}

echo "Building granet tool..."
ya make -r --checkout "$ARCADIA/alice/nlu/granet/tools/granet"

echo "Check out grammars..."
ya make -j0 --checkout "$ARCADIA/alice/nlu/data/snezhana_ru/granet"
