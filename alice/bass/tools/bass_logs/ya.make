PROGRAM()

OWNER(g:bass)

SRCS(
    main.cpp
)

PEERDIR(
    alice/bass/tools/bass_logs/protos
    alice/bass/libs/client
    alice/bass/libs/logging_v2
    alice/library/yt
    alice/library/yt/protos
    library/cpp/compute_graph
    library/cpp/getopt
    library/cpp/protobuf/yt
    library/cpp/scheme
    mapreduce/yt/client
    mapreduce/yt/common
    mapreduce/yt/interface/logging
    mapreduce/yt/util
    library/cpp/cgiparam
)

END()

