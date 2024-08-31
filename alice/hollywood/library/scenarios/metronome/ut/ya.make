UNITTEST_FOR(alice/hollywood/library/scenarios/metronome)

OWNER(
    nkodosov
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/framework/unittest
    alice/hollywood/library/testing
    apphost/lib/service_testing
    library/cpp/testing/gmock_in_unittest
)


SRCS(
    metronome_ut.cpp
)

END()
