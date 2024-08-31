#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ $# < 1 ]]; then
    echo "Usage: $0 FILTER [GRANET_OPTIONS]..."
    echo "Example: $0 turn_on_device"
    exit 1
fi

"$SCRIPT_DIR/../../update_entities.sh" "$SCRIPT_DIR" "$@"
