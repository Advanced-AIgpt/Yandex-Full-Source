LIBRARY()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/global_context
    alice/hollywood/library/nlg
    alice/hollywood/protos
    alice/library/logger
    alice/library/metrics
    apphost/lib/service_testing
    library/cpp/testing/gmock_in_unittest
)

SRCS(
    mock_global_context.cpp
    mock_scenario_run_request_wrapper.cpp
)

END()
