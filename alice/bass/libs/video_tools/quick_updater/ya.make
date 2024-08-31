LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/video_common
    alice/bass/libs/video_content
    alice/bass/libs/ydb_helpers
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_scheme
    ydb/public/sdk/cpp/client/ydb_table
)

SRCS(
    quick_updater_utils.cpp
)

END()

RECURSE(
    ut
)
