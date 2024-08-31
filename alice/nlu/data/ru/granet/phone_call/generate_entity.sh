#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

if [[ $# < 1 ]]; then
    echo "Usage: $0 INPUT_TABLE"
    echo "Example: $0 //home/voice/deemonasd/DIALOG-7617/address_books_distinct_formatted_name"
    exit 1
fi

INPUT_TABLE="$1"
#shift 2

GENERATOR="$ARCADIA/junk/deemonasd/phone_call_quality"

if [[ ! -e "$GENERATOR/phone_call_quality" ]]; then
    "ya make -j24 -r $GENERATOR"
fi

YT_PROXY=hahn ya tool yt read "$INPUT_TABLE" --format '<columns=[formatted_name]>schemaful_dsv' > "$SCRIPT_DIR/address_book.tsv"
cat "$SCRIPT_DIR/address_book.tsv" | "$GENERATOR/phone_call_quality" generate-entity > "$SCRIPT_DIR/address_book.grnt"
