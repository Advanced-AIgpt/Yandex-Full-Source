#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ -z "$1" ]]; then
    echo "Usage: $0 DATASET_FILE_NAME"
    exit 1
fi

declare -A IDS=(
    ["alice3v1.tsv"]="1896081295"
    ["alice4v1.tsv"]="1896082109"
    ["alice5v1.tsv"]="1896082477"
    ["alice6v1.tsv"]="1896083485"
    ["alice3v2.tsv"]="1994875187"
    ["alice4v2.tsv"]="1994875503"
    ["alice5v2.tsv"]="1994876148"
    ["alice6v2.tsv"]="1994877405"
    ["alice3v3.tsv"]="2347626893"
    ["alice4v3.tsv"]="2347627232"
    ["alice5v3.tsv"]="2347627927"
    ["alice6v3.tsv"]="2347631340"
    ["alice3v3_embed.tsv"]="2059602314"
    ["alice4v3_embed.tsv"]="2059603543"
    ["alice5v3_embed.tsv"]="2059160159"
    ["alice6v3_embed.tsv"]="2061852809"
    ["auto5v1.tsv"]="1578357816"
    ["auto5v1.tsv"]="1578357816"
    ["iot4v2.tsv"]="2077402452"
    ["iot5v2.tsv"]="2077535096"
    ["iot_pure5v2.tsv"]="2088221967"
    ["random5v3.tsv"]="1315309724"
    ["random6v3.tsv"]="1313859141"
    ["random7v3.tsv"]="1305010136"
    ["quasar5v1.tsv"]="1050784017"
    ["toloka19all.tsv"]="1362019244"
    ["toloka19uniq5v1.tsv"]="1361994046"
    ["toloka19uniq6v1.tsv"]="1362392863"
)

wget -O - "https://proxy.sandbox.yandex-team.ru/${IDS[$1]}" | tar -xzf - -C "$SCRIPT_DIR"
