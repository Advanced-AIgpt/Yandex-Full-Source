UNITTEST_FOR(alice/hollywood/library/scenarios/music)

OWNER(
    g:megamind
    sparkle
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/testing
    alice/library/json
    library/cpp/testing/gmock_in_unittest
    apphost/lib/service_testing
)

SRCS(
    alice/hollywood/library/scenarios/music/handles/run_prepare/ut/run_prepare_handle_ut.cpp
)

END()
