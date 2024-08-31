PROGRAM(wondercli)

OWNER(g:wonderlogs)

SRCS(
    config.proto
    main.cpp
)

PEERDIR(
    alice/wonderlogs/daily/lib
    alice/wonderlogs/library/common
    alice/wonderlogs/library/protos
    library/cpp/getoptpb
    contrib/libs/protobuf
    mapreduce/yt/client
    mapreduce/yt/common
    library/cpp/getoptpb/proto
)

END()
