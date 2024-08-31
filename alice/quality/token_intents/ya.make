PROGRAM()

OWNER(g:alice_quality)

PEERDIR(
    alice/library/intent_stats/proto
    alice/nlu/libs/request_normalizer
    library/cpp/getopt
    mapreduce/yt/client
    mapreduce/yt/interface
    mapreduce/yt/util
    quality/tools/top/top_reducer
    quality/tools/util
    util/draft
)

SRCS(
    main.cpp
)

END()
