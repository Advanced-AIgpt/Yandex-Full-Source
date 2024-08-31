#!/bin/bash

DIR=$(dirname "$0")
INPUT=$1
SCORE=$2
FREQS=$3
ORIGINAL_MODEL=$4
POOL_DCT=$5
RUSLISTER_MAP=$6
PATTERN_MAP=$7


TMP_DIR=$(mktemp -dt "$(basename $0).XXXXXXXXXX")


bash "$DIR/nn_features_maker.sh" $INPUT $ORIGINAL_MODEL > "$TMP_DIR/nn_features"
python "$DIR/word_features_maker.py" --src $INPUT --dct $POOL_DCT --dst "$TMP_DIR/word_features"
python "$DIR/prod_features_maker.py" --src $INPUT --ruslister-map $RUSLISTER_MAP \
									 --pattern-map $PATTERN_MAP --dst "$TMP_DIR/prod_features"

paste $SCORE $FREQS "$TMP_DIR/nn_features" "$TMP_DIR/word_features" "$TMP_DIR/prod_features"

rm -rf $TMP_DIR
