UNITTEST_FOR(alice/hollywood/library/scenarios/order)

OWNER(
    caesium
    g:alice
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/framework
    alice/hollywood/library/framework/unittest
    alice/hollywood/library/testing
    alice/hollywood/library/scenarios/order/proto
    alice/library/json
    apphost/lib/service_testing
    library/cpp/testing/gmock_in_unittest
)


SRCS(
    order_dispatch_ut.cpp
    order_render_ut.cpp
)

END()