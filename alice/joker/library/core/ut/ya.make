UNITTEST_FOR(alice/joker/library/core)

OWNER(g:bass)

PEERDIR(
    library/cpp/testing/gmock_in_unittest
    library/cpp/testing/unittest
)

SIZE(MEDIUM)

SRCS(
    requests_history_ut.cpp
)

END()
