UNITTEST_FOR(alice/hollywood/library/frame_filler/lib)

OWNER(g:megamind)

PEERDIR(
    alice/library/frame
    alice/library/unittest
    alice/megamind/protos/analytics
    library/cpp/json
    library/cpp/testing/gmock_in_unittest
    apphost/lib/service_testing
)

SRCS(
    frame_filler_ut.cpp
    scenario_state.proto
    analytics_info.proto
)

END()
