OWNER(g:megamind)

UNITTEST_FOR(alice/megamind/library/response)

PEERDIR(
    alice/megamind/library/response
    alice/megamind/library/scenarios/interface
    alice/megamind/library/testing
    library/cpp/testing/gmock_in_unittest
)

SRCS(
    builder_ut.cpp
    response_ut.cpp
    utils_ut.cpp
)

END()
