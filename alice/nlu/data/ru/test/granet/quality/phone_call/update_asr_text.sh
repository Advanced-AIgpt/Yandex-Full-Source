#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

if [[ $# < 2 ]]; then
    echo "Usage: $0 TEST_OR_TRAIN INPUT_TABLE"
    echo "Example: $0 test //home/voice/deemonasd/DIALOG-7617/basket_old_asr_test_dataset"
    exit 1
fi

TEST_OR_TRAIN="$1"
INPUT_TABLE="$2"

GENERATOR="$ARCADIA/junk/deemonasd/phone_call_quality"
if [[ ! -e "$GENERATOR/phone_call_quality" ]]; then
    "ya make -j24 -r $GENERATOR"
fi

YT_PROXY=hahn ya tool yt read "$INPUT_TABLE" --format '<columns=[weight;asr_text;formatted_name]>schemaful_dsv' > "$SCRIPT_DIR/dataset_$TEST_OR_TRAIN.tsv"
cat "$SCRIPT_DIR/dataset_$TEST_OR_TRAIN.tsv" | "$GENERATOR/phone_call_quality" generate-dataset > "$SCRIPT_DIR/target/basket_asr_$TEST_OR_TRAIN.tsv"
