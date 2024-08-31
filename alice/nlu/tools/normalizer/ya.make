PROGRAM()

OWNER(
    smirnovpavel
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/request_normalizer
    mapreduce/yt/client
    mapreduce/yt/interface
    mapreduce/yt/interface/logging
    library/cpp/yson/node
    mapreduce/yt/util
    library/cpp/getopt/small
)

SRCS(
    main.cpp
)

END()
