UNITTEST_FOR(alice/megamind/library/apphost_request)

OWNER(
    g:megamind
)

PEERDIR(
    alice/library/json
    alice/library/unittest
    alice/megamind/library/testing
)

SRCS(
    item_adapter_ut.cpp
    request_builder_ut.cpp
    test.proto
    util_ut.cpp
)

REQUIREMENTS(ram:10)

END()
