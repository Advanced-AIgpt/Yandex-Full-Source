#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

if [[ $# < 1 ]]; then
    echo "Usage:   $0 dataset_name"
    echo "Example: $0 pool/base.tsv"
    exit 1
fi

GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"

if [[ ! -e "$GRANET" ]]; then
    "$ARCADIA/alice/nlu/data/ru/test/granet/prepare.sh"
fi

"$GRANET" dataset update-entities -i $1 --embeddings
