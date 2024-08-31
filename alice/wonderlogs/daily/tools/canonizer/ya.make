PROGRAM(canonizer)

OWNER(g:wonderlogs)

SRCS(
    config.proto
    main.cpp
)

PEERDIR(
    alice/wonderlogs/protos
    alice/wonderlogs/daily/lib
    alice/wonderlogs/library/protos
    library/cpp/json
    contrib/libs/protobuf
    alice/library/json
    library/cpp/json
    library/cpp/json/yson
    library/cpp/yson/node
    library/cpp/getoptpb
    mapreduce/yt/client
    mapreduce/yt/common
    library/cpp/getoptpb/proto
)

END()
