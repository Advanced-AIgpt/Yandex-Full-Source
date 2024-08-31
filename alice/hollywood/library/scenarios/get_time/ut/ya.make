UNITTEST_FOR(alice/hollywood/library/scenarios/get_time)

OWNER(
    alexanderplat
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/framework
    alice/hollywood/library/framework/unittest
    alice/hollywood/library/testing
    alice/hollywood/library/scenarios/get_time/proto
    alice/library/json
    apphost/lib/service_testing
    library/cpp/testing/gmock_in_unittest
)


SRCS(
    get_time_dispatch_ut.cpp
)

END()
