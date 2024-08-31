UNITTEST_FOR(alice/hollywood/library/scenarios/show_traffic_bass)

OWNER(
    yagafarov
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/context
    alice/hollywood/library/nlg
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/show_traffic_bass/nlg
    alice/hollywood/library/scenarios/show_traffic_bass/proto
    alice/hollywood/library/testing
    alice/library/json
    alice/library/logger
    alice/library/unittest
    alice/megamind/protos/scenarios
    apphost/lib/service_testing
    library/cpp/json
    library/cpp/testing/gmock_in_unittest
)

SRCS(
    alice/hollywood/library/scenarios/show_traffic_bass/render_with_bass_ut.cpp
)

DATA(arcadia/alice/hollywood/library/scenarios/show_traffic_bass/ut/data/)

END()
