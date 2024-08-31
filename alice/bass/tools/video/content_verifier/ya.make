PROGRAM()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/logging_v2
    alice/bass/libs/video_common
    alice/bass/libs/ydb_config
    alice/bass/libs/ydb_helpers
    alice/bass/libs/ydb_kv
    alice/bass/libs/ydb_kv/protos
    library/cpp/getopt
    library/cpp/string_utils/scan
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
)

SRCS(
    main.cpp
)

END()
