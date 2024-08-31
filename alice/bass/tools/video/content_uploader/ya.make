PROGRAM()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/logging_v2
    alice/bass/libs/video_common
    alice/bass/libs/video_content
    alice/bass/libs/video_content/protos
    alice/bass/libs/ydb_config
    alice/bass/libs/ydb_helpers
    alice/bass/libs/ydb_kv
    alice/bass/libs/ydb_kv/protos
    library/cpp/getopt
    mapreduce/yt/client
    mapreduce/yt/common
    mapreduce/yt/library/table_schema
    mapreduce/yt/util
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
)

SRC(main.cpp)

END()
