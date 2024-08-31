UNITTEST_FOR(alice/hollywood/library/scenarios/weather/s3_animations)

OWNER(
    sparkle
    g:alice
)

PEERDIR(
    alice/hollywood/library/s3_animations
    alice/library/json
    alice/megamind/protos/scenarios
    library/cpp/testing/gmock_in_unittest
)

SRCS(
    alice/hollywood/library/scenarios/weather/s3_animations/s3_animations_ut.cpp
)

END()
