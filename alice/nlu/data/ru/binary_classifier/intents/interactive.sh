#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

if [[ $# < 1 ]]; then
    echo "Usage: $0 CONFIG_PATH"
    echo "Example: $0 shortcut/muzpult/config.json"
    exit 1
fi

CONFIG_PATH="$1"
CONFIG_DIR="$(dirname "$CONFIG_PATH")"
shift 1

ya() {
    "$ARCADIA/ya" "$@"
}

echo "Downloading datasets..."
ya make -r --checkout "$CONFIG_DIR"
ya make -r --checkout "$ARCADIA/alice/nlu/data/ru/binary_classifier/pool"

echo "Building binary classifier tool..."
ya make -r --checkout "$ARCADIA/alice/nlu/tools/binary_classifier"

echo "Run binary classifier tool"
"$ARCADIA/alice/nlu/tools/binary_classifier/binary_classifier" interactive \
    --config "$CONFIG_PATH" \
    --run-learn \
    "$@"
