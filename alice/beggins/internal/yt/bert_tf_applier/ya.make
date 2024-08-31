PROGRAM()

OWNER(dan4fedor)

SRCS(
    applier_map.cpp
    config.proto
)

PEERDIR(
    alice/beggins/internal/bert_tf
    library/cpp/getoptpb
    mapreduce/yt/client
    mapreduce/yt/interface
)

END()
