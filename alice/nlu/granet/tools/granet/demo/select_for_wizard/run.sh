#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"
ya() {
    "$ARCADIA/ya" "$@"
}

GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"
GRAMMAR="$ARCADIA/alice/nlu/data/wizard_ru/granet/main.grnt"
FORM="wizard.demo_hello_world"

ya make -j0 --checkout "$ARCADIA/alice/nlu/data/wizard_ru/granet"
ya make -r --checkout "$ARCADIA/alice/nlu/granet/tools/granet"

cd "$SCRIPT_DIR"
mkdir -p result

echo "============ prepare dataset ============"

"$GRANET" dataset create \
    -i dataset.tsv \
    -o result/prepared.tsv \
    --lang ru \
    --wizard

echo "============ select ============"

"$GRANET" dataset select \
    -i result/prepared.tsv \
    -p result/positive.tsv \
    --lang ru \
    --wizard \
    --grammar "$GRAMMAR" \
    --keep-extra yes \
    --form "$FORM"
