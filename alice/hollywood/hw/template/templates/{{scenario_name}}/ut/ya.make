UNITTEST_FOR(alice/hollywood/library/scenarios/{{scenario_name}})

OWNER(
    {{username}}
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/framework
    alice/hollywood/library/framework/unittest
    alice/hollywood/library/testing
    alice/hollywood/library/scenarios/{{scenario_name}}/proto
    alice/library/json
    apphost/lib/service_testing
    library/cpp/testing/gmock_in_unittest
)


SRCS(
    {{scenario_name}}_dispatch_ut.cpp
    {{scenario_name}}_render_ut.cpp
)

END()
