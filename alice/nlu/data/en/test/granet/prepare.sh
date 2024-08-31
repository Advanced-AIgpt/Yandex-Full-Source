#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

ya() {
    "$ARCADIA/ya" "$@"
}

echo "Building granet tool..."
ya make -r "$ARCADIA/alice/nlu/granet/tools/granet"

#echo "Downloading datasets..."
#ya make -r "$ARCADIA/alice/nlu/data/ru/test/pool"

echo "Check out grammars..."
ya make -j0 "$ARCADIA/alice/nlu/data/en/granet"
