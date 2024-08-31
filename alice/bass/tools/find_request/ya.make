PROGRAM()

OWNER(
    g:bass
)

SRCS(
    main.cpp
)

PEERDIR(
    alice/bass/tools/bass_logs/protos
    alice/library/yt/protos
    mapreduce/yt/util
    mapreduce/yt/client
    mapreduce/yt/library/operation_tracker
    mapreduce/yt/interface/logging
    library/cpp/getopt
    library/cpp/json
    library/cpp/timezone_conversion
)

END()
