LIBRARY()

OWNER(
    d-dima
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/config
    alice/hollywood/library/framework
    alice/hollywood/library/framework/core
    alice/hollywood/library/framework/proto
    alice/hollywood/library/testing
    alice/library/metrics/sensors_dumper
    alice/library/unittest
    alice/megamind/protos/scenarios
    apphost/lib/service_testing
    contrib/libs/protobuf
    library/cpp/json
    library/cpp/string_utils/base64
    library/cpp/testing/gmock_in_unittest
)

SRCS(
    node_caller_testing.cpp
    scenario_factory_testing.cpp
    test_environment.cpp
    test_globalcontext.cpp
    test_nodes.cpp
)

END()
