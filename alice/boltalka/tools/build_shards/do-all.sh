#!/bin/bash -x
INPUT=$1 # //home/cvtest/alipov/twitter_all_replies.filtered_by_both
MODEL=$2 # ~/extsearch_models/insight_tw_c3/model
EMB=$3 # //home/cvtest/alipov/twitter_all_replies.filtered_by_both.emb
OUTPUT=$4 # rus_lister_index
SHARDS=$5 # 5

DIR=/home/alipov/arcadia/alice/boltalka/tools/build_shards

$DIR/build_shards apply-dssm -i $INPUT -o $EMB -m $MODEL && \
$DIR/build_shards assign-shards -i $EMB -o $EMB.shard_data -n $SHARDS && \
$DIR/build_shards download-shard -i $EMB.shard_data -o $OUTPUT -s 0

# shard_data //home/cvtest/alipov/twitter_all_replies.filtered_by_both
