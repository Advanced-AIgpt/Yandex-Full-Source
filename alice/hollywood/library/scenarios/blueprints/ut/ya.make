UNITTEST_FOR(alice/hollywood/library/scenarios/blueprints)

OWNER(
    d-dima
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/framework
    alice/hollywood/library/framework/unittest
    alice/hollywood/library/testing
    alice/hollywood/library/scenarios/blueprints/proto
    alice/library/json
    apphost/lib/service_testing
    library/cpp/testing/gmock_in_unittest
)


SRCS(
    blueprints_dispatch_ut.cpp
    blueprints_render_ut.cpp
)

END()