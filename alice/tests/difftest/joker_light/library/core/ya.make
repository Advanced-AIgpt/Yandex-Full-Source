LIBRARY()

OWNER(sparkle)

PEERDIR(
    alice/joker/library/log
    library/cpp/http/misc
    library/cpp/http/server
    library/cpp/neh
    library/cpp/openssl/crypto
    library/cpp/scheme
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
)

SRCS(
    client.cpp
    config.cpp
    config.sc
    context.cpp
    http_context.cpp
    server.cpp
    session.cpp
    status.cpp
    ydb.cpp
)

END()
