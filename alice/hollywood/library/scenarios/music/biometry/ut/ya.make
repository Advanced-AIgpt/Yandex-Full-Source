UNITTEST_FOR(alice/hollywood/library/scenarios/music/biometry)

OWNER(
    sparkle
    g:hollywood
    g:alice
)

DATA(arcadia/alice/hollywood/library/scenarios/music/biometry/ut/data/)

SRCS(
    alice/hollywood/library/scenarios/music/biometry/process_biometry_ut.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music/biometry
    alice/library/json
    alice/protos/endpoint/capabilities/bio
    library/cpp/resource
    library/cpp/testing/unittest
    apphost/lib/service_testing
)

END()
