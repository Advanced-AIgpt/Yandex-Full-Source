#!/usr/bin/env bash

CHECKPOINT_PATH="$1"
OUTPUT_PATH="$2"

#ya make dict/mt/make/tools/tfnn/convert_tfnn_to_mtd -r && \
~/code/arcadia/dict/mt/make/tools/tfnn/convert_tfnn_to_mtd/convert_tfnn_to_mtd \
    ${CHECKPOINT_PATH} \
    ${OUTPUT_PATH} \
    --arch transformer \
    --num-heads 8  \
    --max-rel-pos 0
