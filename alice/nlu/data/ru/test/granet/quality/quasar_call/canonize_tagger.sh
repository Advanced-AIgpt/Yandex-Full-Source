#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ $# < 1 ]]; then
    echo "Usage: $0 FILTER [GRANET_OPTIONS]..."
    echo "Examples:"
    echo "  $0 alarm_set    # canonize tagger results for specified test cases"
    echo "  $0 ''           # canonize tagger results for all test cases"
    exit 1
fi

"$SCRIPT_DIR/../../canonize_tagger.sh" "$SCRIPT_DIR" "$@"
