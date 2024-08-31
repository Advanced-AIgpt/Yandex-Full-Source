UNITTEST_FOR(alice/megamind/library/config)

OWNER(g:alice)

SRCS(
    config_ut.cpp
)

PEERDIR(
    alice/megamind/library/config
    contrib/libs/protobuf
    library/cpp/json
    library/cpp/protobuf/json
    library/cpp/protobuf/util
    library/cpp/resource
    library/cpp/testing/unittest
)

END()
