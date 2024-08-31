UNITTEST_FOR(alice/megamind/library/requestctx)

OWNER(
    g:megamind
)

PEERDIR(
    alice/library/logger/proto
    alice/library/metrics
    alice/megamind/library/testing
    library/cpp/testing/unittest
)

SRCS(
    rtlogtoken_ut.cpp
)

REQUIREMENTS(ram:10)

END()
