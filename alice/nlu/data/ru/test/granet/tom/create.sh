#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"

if [[ $# < 3 ]]; then
    echo "Usage: $0 BATCH_NAME FORM_NAME YT_PATH [EXTRA_OPTIONS]..."
    echo "Example: $0 equalizer_less_bass alice.equalizer.less_bass //home/alice-ue2e/baskets_from_logs/baskets/ALICETOM-70__mysov-gr__less_bass/basket_dev"
    exit 1
fi

ya() {
    "$ARCADIA/ya" "$@"
}

BATCHES_DIR="$ARCADIA/alice/nlu/data/ru/test/granet/tom"

echo "Building create_batch tool..."
ya make -r "$ARCADIA/alice/nlu/granet/tools/create_batch"

"$ARCADIA/alice/nlu/granet/tools/create_batch/create_batch" \
    create-tom \
    --owner "$(whoami)" \
    --batches-dir "$BATCHES_DIR" \
    --batch-name "$1" \
    --form-name "$2" \
    --yt-path "$3" \
    "${@:4}"

echo
"$ARCADIA/alice/nlu/data/ru/test/granet/prepare.sh"

echo
echo "Update entities..."
"$BATCHES_DIR/$1/update_entities.sh" "tom_quality"

echo
echo "Canonize current results..."
"$BATCHES_DIR/$1/canonize.sh" ""

echo
echo "Test..."
"$BATCHES_DIR/$1/test.sh" ""

echo
echo "Batch successfully created at $BATCHES_DIR/$1"
