OWNER(
    dan-anastasev
    g:alice_quality
)

PROGRAM()

PEERDIR(
    alice/boltalka/libs/text_utils
    kernel/dssm_applier/nn_applier/lib
    library/cpp/dot_product
    library/cpp/getopt/small
    library/cpp/langs
    library/cpp/yson/node
    mapreduce/yt/client
    mapreduce/yt/interface
    mapreduce/yt/interface/logging
    mapreduce/yt/library/lambda
    mapreduce/yt/util
)

SRCS(
    main.cpp
)

END()

RECURSE(data)
