LIBRARY()

OWNER(g:wonderlogs)

SRCS(
    names.cpp
    utils.cpp
)

PEERDIR(
    alice/library/json
    library/cpp/regex/pire
    library/cpp/yson/node
    library/cpp/openssl/crypto
    library/cpp/json/writer
    contrib/libs/protobuf
)

END()

RECURSE_FOR_TESTS(ut)
