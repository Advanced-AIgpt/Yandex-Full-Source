UNITTEST_FOR(alice/hollywood/library/request)

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/request/ut/proto
    library/cpp/testing/unittest
    apphost/lib/service_testing
)

SRCS(
    alice/hollywood/library/request/request_ut.cpp
)

END()
