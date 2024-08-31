PROGRAM()

ALLOCATOR(B)

OWNER(
    avitella
)

PEERDIR(
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    library/cpp/cgiparam
    library/cpp/getopt
    library/cpp/json
    library/cpp/json/yson
    library/cpp/logger/global
    library/cpp/protobuf/json
    mapreduce/yt/client
    mapreduce/yt/interface
    mapreduce/yt/util
    search/multiplexer
)

SRCS(
    main.cpp
    collapsed_log.proto
)

GENERATE_ENUM_SERIALIZATION(main.h)

END()
