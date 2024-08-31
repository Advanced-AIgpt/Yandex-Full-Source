#!/bin/bash -x
mkdir -p workindex/insight_c3_rus_lister && \
rm -f workindex/index* && \
cp ~/extsearch_models/insight_tw_c1/model workindex/insight_c3_rus_lister/ && \
( cat ~/datasets/gogol.txt | ./parse_nlu.py | sed -re 's/^/context_0=/; s/\t/\treply=/; s/$/\tcontext_1=\tcontext_2=/' | /usr/bin/python /usr/bin/yt --proxy hahn write --table //home/cvtest/alipov/gogol --format dsv ) && \
~/arcadia/alice/boltalka/tools/build_shards/do-all.sh //home/cvtest/alipov/gogol ~/extsearch_models/insight_tw_c1/model //home/cvtest/alipov/gogol.emb workindex/insight_c3_rus_lister 1 && \
rm workindex/insight_c3_rus_lister/reply.* workindex/insight_c3_rus_lister/context.* && \
~/arcadia/library/cpp/hnsw/tools/build_dense_vector_index/build_dense_vector_index -v workindex/insight_c3_rus_lister/context_and_reply.vec -d 600 -t float -D dot_product -o workindex/insight_c3_rus_lister/context_and_reply.index -m 128 -b 30000 -s 30000 -e 30000 && \
( cat workindex/insight_c3_rus_lister/context_and_reply.txt | ~/arcadia/alice/boltalka/extsearch/indexer/indexer -d workindex ) && \
sky share workindex
