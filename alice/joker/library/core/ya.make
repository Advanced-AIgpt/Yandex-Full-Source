LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/joker/library/log
    alice/joker/library/proto
    alice/joker/library/s3
    alice/joker/library/session
    alice/joker/library/status
    alice/joker/library/stub
    library/cpp/cache
    library/cpp/cgiparam
    library/cpp/config
    library/cpp/http/misc
    library/cpp/http/server
    library/cpp/json
    library/cpp/neh
    library/cpp/openssl/crypto
    library/cpp/protobuf/util
    library/cpp/scheme
    library/cpp/threading/future
    library/cpp/timezone_conversion
    library/cpp/uri
    library/cpp/deprecated/atomic
)

SRCS(
    backend_fs.cpp
    backend_s3.cpp
    client.cpp
    config.cpp
    config.sc
    ctrl_session.cpp
    globalctx.cpp
    http_session.cpp
    joker.cpp
    memory_storage.cpp
    request.cpp
    requests_history.cpp
    server.cpp
    session.cpp
)

GENERATE_ENUM_SERIALIZATION(http_session.h)

END()

RECURSE_FOR_TESTS(
    ut
)
