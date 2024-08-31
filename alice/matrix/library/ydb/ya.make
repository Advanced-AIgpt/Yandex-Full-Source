LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    storage.cpp
)

PEERDIR(
    alice/matrix/library/config
    alice/matrix/library/logging
    alice/matrix/library/metrics

    infra/libs/outcome

    ydb/public/sdk/cpp/client/ydb_scheme
    ydb/public/sdk/cpp/client/ydb_table
)

END()
