PROGRAM()

OWNER(g:wonderlogs)

SRCS(
    config.proto
    main.cpp
)

PEERDIR(
    alice/wonderlogs/protos
    alice/wonderlogs/rt/processors/static_wonderlogs_creator/protos

    library/cpp/getoptpb
    library/cpp/getoptpb/proto

    mapreduce/yt/client
)

END()
