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

cd "$SCRIPT_DIR"

"$GRANET" dataset select \
    -i result/prepared.tsv \
    -p result/positive.tsv \
    --lang ru \
    --wizard \
    --grammar "$GRAMMAR" \
    --keep-extra yes \
    --form "$FORM"
