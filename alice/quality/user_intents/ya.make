PROGRAM()

OWNER(g:alice_quality)

PEERDIR(
    alice/quality/user_intents/proto
    library/cpp/getopt
    library/cpp/timezone_conversion
    mapreduce/yt/client
    mapreduce/yt/interface
    mapreduce/yt/util
    quality/tools/top/top_reducer
    quality/tools/util
    util/draft
)

SRCS(
    intents.cpp
    main.cpp
)

END()
