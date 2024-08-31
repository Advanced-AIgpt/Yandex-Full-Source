#!/bin/bash

DIR=$(dirname "$0")
INPUT=$1
ORIGINAL_MODEL=$2

LAYER_SUFFIX="embedding"


TMP_DIR=$(mktemp -dt "$(basename $0).XXXXXXXXXX")


cat $INPUT | python "$DIR/map_punct.py" > "$TMP_DIR/input"
bash "$DIR/calc_embeddings.sh" $LAYER_SUFFIX "$TMP_DIR/input" $ORIGINAL_MODEL > "$TMP_DIR/emb_ori300"

python "$DIR/dist_features_maker.py" --input "$TMP_DIR/emb_ori300"

rm -rf $TMP_DIR
