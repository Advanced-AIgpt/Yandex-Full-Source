UNITTEST_FOR(alice/hollywood/library/scenarios/random_number)

OWNER(
    d-dima
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/framework/unittest
    alice/hollywood/library/testing
    apphost/lib/service_testing
    library/cpp/testing/gmock_in_unittest
)


SRCS(
    random_number_ut.cpp
)

END()
