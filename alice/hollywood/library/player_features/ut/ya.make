UNITTEST_FOR(alice/hollywood/library/player_features)

OWNER(g:alice)

PEERDIR(
    alice/hollywood/library/player_features
    alice/library/json
    alice/library/unittest
    alice/megamind/protos/scenarios
    apphost/lib/service_testing
)

SRCS(
    player_features_ut.cpp
)

END()
