#!/usr/bin/env bash

print_usage() {
    echo "Usage: $0 YT_PROXY INPUT_TABLE_PATH OUTPUT_TABLE_PATH BEGEMOT_CACHE_PATH"
    echo -e "\tYT_PROXY\tYT claster name."
    echo -e "\tINPUT_TABLE_PATH\tA cypress path for an input table. The table must contain column named 'text', which will be treated as queries."
    echo -e "\tOUTPUT_TABLE_PATH\tA cypress path for the output table."
    echo -e "\tBEGEMOT_CACHE_PATH\tA cypress path to store Begemot cache."
}

if { { [ -z "$1" ] || [ -z "$2" ]; } || [ -z "$3" ]; } || [ -z "$4" ]; then
    print_usage
    exit 1
fi

set -e

YT_PROXY=$1
INPUT_TABLE_PATH=$2
OUTPUT_TABLE_PATH=$3
BEGEMOT_CACHE_PATH=$4

ARCADIA="$(readlink -f $(dirname $(readlink -f $BASH_SOURCE))/../../../)"
NORMALIZER_DIR=$ARCADIA/alice/nlu/tools/normalizer
MAPPER_DIR=$ARCADIA/tools/wizard_yt/begemot_reducer/mapper
SHARD_PATH=$ARCADIA/search/begemot/data/Megamind/search/wizard/data/wizard
BEGEMOT_CONFIG='{"binary": false, "result_type": "begemot", "requested_rules": ["AliceEmbeddings", "AliceSampleFeatures"]}'

$ARCADIA/ya make -r --checkout $NORMALIZER_DIR
$ARCADIA/ya make -r --checkout $MAPPER_DIR
$ARCADIA/ya make -r --checkout $ARCADIA/search/begemot/data/Megamind

normalize() {
    echo "Normalization started..."
    $NORMALIZER_DIR/normalizer \
        --proxy $YT_PROXY \
        --input $INPUT_TABLE_PATH \
        --output $OUTPUT_TABLE_PATH \
        --input-column text \
        --output-column text \
        --use-nlu-tags
    INPUT_TABLE_PATH=$OUTPUT_TABLE_PATH
    echo "Normalization finished!"
}

extract_sample_features() {
    echo "Sample features extraction started..."
    $MAPPER_DIR/mapper \
        --proxy $YT_PROXY \
        --input $INPUT_TABLE_PATH \
        --output $OUTPUT_TABLE_PATH \
        --direct text \
        --begemot_config "$BEGEMOT_CONFIG" \
        --shard_rule AliceSampleFeatures \
        --shard_path "$SHARD_PATH" \
        --cypress_shard_cache $BEGEMOT_CACHE_PATH
    INPUT_TABLE_PATH=$OUTPUT_TABLE_PATH
    echo "Sample features extraction finished!"
}

normalize
extract_sample_features
