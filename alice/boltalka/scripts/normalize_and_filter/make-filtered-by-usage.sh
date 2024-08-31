#!/bin/bash
set -x
OUT=$1 # insight_c3_usage
YT_OUT=$2 # //home/cvtest/alipov/twitter_c3
MODEL=$3 # ~/extsearch_models/insight_tw_c3/model
mkdir $OUT && \
./generate_replies_for_index.py --src //home/cvtest/alipov/twitter_all_sequences --dst $YT_OUT && \
./filter_by_dict.py --dict-file rus-lister.dict --bad-eng-dict-file ../../filters/bad_eng.txt --dict-ratio 1.0 --src $YT_OUT --dst $YT_OUT.rus_lister && \
./filter_by_whitelist.py --src $YT_OUT.rus_lister --whitelist //home/cvtest/alipov/twitter_all_replies.filtered_by_suggest --dst $YT_OUT.rus_lister.suggest && \
./calc_reply_freq.py --src //home/cvtest/alipov/twitter_all_sequences --dst //home/cvtest/alipov/twitter.replies.usage && \
./filter_by_whitelist.py --src $YT_OUT.rus_lister.suggest --whitelist //home/cvtest/alipov/twitter.replies.usage --dst $YT_OUT.rus_lister.suggest.usage && \
./build_shards.sh $YT_OUT.rus_lister.suggest.usage $MODEL $YT_OUT.rus_lister.suggest.usage.emb $OUT 2 && \
( cat $OUT/context_and_reply.txt | cut -f4 | python ./remove_emoticons.py --strip-nonstandard-chars | ./lower_and_separate_punct.py > $OUT/replies.normed.txt )
