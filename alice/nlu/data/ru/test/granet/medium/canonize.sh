#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ $# < 1 ]]; then
    echo "Usage: $0 FILTER [GRANET_OPTIONS]..."
    echo "Examples:"
    echo "  $0 alice.demo   # canonize specified test cases"
    echo "  $0 ''           # canonize all test cases"
    exit 1
fi

"$SCRIPT_DIR/../canonize.sh" "$SCRIPT_DIR" "$@"
