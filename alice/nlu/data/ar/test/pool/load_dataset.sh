#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ -z "$1" ]]; then
    echo "Usage: $0 DATASET_FILE_NAME"
    exit 1
fi

declare -A IDS=(
    ["alice_ar_v1.tsv"]="2887186842"
    ["alice_ar_v2.tsv"]="2932490352"
    ["alice_ar_v3.tsv"]="2986683777"
)

wget -O - "https://proxy.sandbox.yandex-team.ru/${IDS[$1]}" | tar -xzf - -C "$SCRIPT_DIR"
