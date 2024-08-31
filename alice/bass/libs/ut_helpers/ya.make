LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/ydb_helpers
    alice/library/unittest
    ydb/public/sdk/cpp/client/ydb_scheme
    ydb/public/sdk/cpp/client/ydb_table
)

SRCS(
    mocks.cpp
    test_with_ydb.cpp
)

END()
