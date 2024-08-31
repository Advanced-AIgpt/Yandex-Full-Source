#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

ya() {
    "$ARCADIA/ya" "$@"
}

if [[ $# < 1 ]]; then
    echo "Usage: $0 FORM"
    echo "Example: $0 alice.random_number"
    exit 1
fi

FORM="$1"
shift 1

GRANET="$ARCADIA/alice/nlu/granet/tools/granet/granet"
GRAMMAR="$ARCADIA/alice/nlu/data/ru/granet/main.grnt"
SYNONYMS="$ARCADIA/search/wizard/data/wizard/AliceThesaurus/synonyms.gzt.bin"
SYNONYMS_FIXLIST="$ARCADIA/search/wizard/data/wizard/AliceThesaurus/fixlist.gzt.bin"

if [[ ! -e "$GRANET" ]]; then
    "$ARCADIA/alice/nlu/data/ru/test/granet/prepare.sh"
fi
if [[ ! -e "$SYNONYMS" ]]; then
    ya make --checkout -r $ARCADIA/search/wizard/data/wizard/AliceThesaurus
fi
if [[ ! -e "$SYNONYMS_FIXLIST" ]]; then
    ya make --checkout -r $ARCADIA/search/wizard/data/wizard/AliceThesaurus
fi

"$GRANET" debug synonyms \
    -g "$GRAMMAR" \
    --form "$FORM" \
    --synonyms "$SYNONYMS" \
    --synonyms-fixlist "$SYNONYMS_FIXLIST" \
    "$@"
