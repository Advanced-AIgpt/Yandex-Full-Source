UNITTEST_FOR(alice/hollywood/library/scenarios/onboarding)

OWNER(
    vitamin-ca
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/framework
    alice/hollywood/library/framework/unittest
    alice/hollywood/library/testing
    alice/hollywood/library/scenarios/onboarding/proto
    alice/library/json
    apphost/lib/service_testing
    library/cpp/testing/gmock_in_unittest
)


SRCS(
    onboarding_dispatch_ut.cpp
    onboarding_render_ut.cpp
)

END()