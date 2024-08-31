UNITTEST_FOR(alice/megamind/library/request)

OWNER(g:megamind)

PEERDIR(
    alice/library/json
    alice/megamind/library/models/buttons
    alice/megamind/library/testing
    alice/megamind/library/util
    library/cpp/json
    library/cpp/testing/unittest
)

SRCS(
    alice/megamind/library/request/request_ut.cpp
)

REQUIREMENTS(ram:9)

END()
