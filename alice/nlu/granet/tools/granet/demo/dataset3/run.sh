#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"
cd "$SCRIPT_DIR"

mkdir -p dst

dataset_two_step_creation() {
    echo "============ dataset_two_step_creation ============"
    ../../granet dataset create \
        -i src.tsv \
        -o dst/dataset.tsv \
        --normalize \
        --lang ru \
        --no-entities

    ../../granet dataset update-entities \
        -i dst/dataset.tsv \
        --lang ru \
        --missing
}

dataset_two_step_creation
