UNITTEST_FOR(alice/hollywood/library/scenarios/bluetooth)

OWNER(
    mihajlova
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/framework
    alice/hollywood/library/framework/unittest
    alice/hollywood/library/testing
    alice/hollywood/library/scenarios/bluetooth/proto
    alice/library/json
    apphost/lib/service_testing
    library/cpp/testing/gmock_in_unittest
)


SRCS(
    bluetooth_dispatch_ut.cpp
    bluetooth_render_ut.cpp
)

END()