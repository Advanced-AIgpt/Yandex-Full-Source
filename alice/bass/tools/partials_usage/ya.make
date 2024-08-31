PROGRAM()

OWNER(g:megamind)

PEERDIR(
    mapreduce/yt/client
    mapreduce/yt/interface
    mapreduce/yt/interface/protos
    mapreduce/yt/library/operation_tracker
    mapreduce/yt/util
    library/cpp/json
    library/cpp/getopt
    library/cpp/protobuf/yt
    library/cpp/threading/future
    library/cpp/timezone_conversion
)

SRCS(
    partials_usage.cpp
    data.proto
)

END()
