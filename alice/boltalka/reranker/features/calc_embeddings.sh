#!/bin/bash

DIR=$(dirname "$0")
LAYER_SUFFIX=$1
INPUT=$2
MODEL=$3

TMP_DIR=$(mktemp -dt "$(basename $0).XXXXXXXXXX")

CONTEXT_LAYER="query_$LAYER_SUFFIX"
REPLY_LAYER="doc_$LAYER_SUFFIX"


cut -f1,2,3 $INPUT > "$TMP_DIR/query"
cut -f4 $INPUT > "$TMP_DIR/reply"
cut -f5,6,7 $INPUT > "$TMP_DIR/context"

paste <(bash "$DIR/../calc_context_embeddings.sh" $CONTEXT_LAYER "$TMP_DIR/query" $MODEL) \
	  <(bash "$DIR/../calc_reply_embeddings.sh" $REPLY_LAYER "$TMP_DIR/reply" $MODEL) \
	  <(bash "$DIR/../calc_context_embeddings.sh" $CONTEXT_LAYER "$TMP_DIR/context" $MODEL)

rm -rf $TMP_DIR
