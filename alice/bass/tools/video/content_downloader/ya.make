PROGRAM()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/logging_v2
    alice/bass/libs/tvm2/ticket_cache
    alice/bass/libs/video_common
    alice/bass/libs/video_content
    alice/bass/libs/video_content/protos
    alice/library/yt
    library/cpp/getopt
    library/cpp/protobuf/yt
    library/cpp/string_utils/scan
    library/cpp/scheme
    mapreduce/yt/client
    mapreduce/yt/common
    mapreduce/yt/library/table_schema
    mapreduce/yt/util
    robot/jupiter/protos/compatibility
)

SRCS(
    main.cpp
    utils.cpp
)

END()
