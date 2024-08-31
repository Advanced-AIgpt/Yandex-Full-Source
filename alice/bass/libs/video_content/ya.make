LIBRARY()

OWNER(
    g:bass
    g:smarttv
)

PEERDIR(
    alice/bass/libs/logging_v2
    alice/bass/libs/video_content/protos
    alice/bass/libs/ydb_helpers
    ydb/public/sdk/cpp/client/ydb_scheme
    ydb/public/sdk/cpp/client/ydb_table
)

SRC(common.cpp)

END()

RECURSE(
    protos
)
