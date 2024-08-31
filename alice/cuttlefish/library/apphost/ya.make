LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    admin_handle_listener.cpp
    async_input_request_handler.cpp
    item_parser.cpp
    metrics_services.cpp
)

PEERDIR(
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/metrics

    apphost/api/service/cpp

    library/cpp/svnversion
    library/cpp/threading/atomic
    library/cpp/threading/future
)

END()

RECURSE_FOR_TESTS(
    test_proto
    ut
)
