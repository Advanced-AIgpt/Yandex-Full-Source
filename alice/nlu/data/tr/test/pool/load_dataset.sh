#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ -z "$1" ]]; then
    echo "Usage: $0 DATASET_FILE_NAME"
    exit 1
fi

declare -A IDS=(
    ["random3v1.tsv"]="1750483349"
    ["random4v1.tsv"]="1750473040"
    ["random5v1.tsv"]="1117908137"
    ["random6v1.tsv"]="1118845504"
)

wget -O - "https://proxy.sandbox.yandex-team.ru/${IDS[$1]}" | tar -xzf - -C "$SCRIPT_DIR"
