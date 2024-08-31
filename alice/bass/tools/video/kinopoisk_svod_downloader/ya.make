PROGRAM()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/logging_v2
    alice/bass/libs/video_common
    alice/bass/libs/video_content
    alice/bass/libs/video_content/protos
    alice/bass/libs/ydb_config
    alice/bass/libs/ydb_helpers
    library/cpp/getopt
    library/cpp/scheme
    mapreduce/yt/client
    mapreduce/yt/interface/protos
    mapreduce/yt/util
    ydb/public/sdk/cpp/client/ydb_driver
)

SRCS(
    kp_genres.cpp
    main.cpp
)

END()
