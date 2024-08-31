LIBRARY()

OWNER(sparkle)

PEERDIR(
    alice/joker/library/log
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
)

SRCS(
    ydb.cpp
)

END()
