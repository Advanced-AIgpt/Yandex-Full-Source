LIBRARY()

OWNER(paxakor)

NEED_REVIEW()

SRCS(
    app.cpp
    app_config.cpp

    async_call.cpp
    async_call_state.cpp
    async_service.cpp
    async_server.cpp

    metadata.cpp

    frame_converter.cpp

    subsystem_admin.cpp
    subsystem_apphost.cpp
    subsystem_grpc.cpp
    subsystem_logging.cpp
    subsystem_metrics.cpp

    test_devices.cpp

    config.proto
)

PEERDIR(
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/metrics
    alice/cuttlefish/library/apphost

    alice/gproxy/library/protos
    alice/gproxy/library/events

    alice/library/json
    alice/library/logger/proto
    alice/megamind/protos/common

    apphost/api/client
    contrib/libs/googleapis-common-protos
    contrib/libs/grpc
    contrib/libs/protobuf

    library/cpp/getoptpb

    voicetech/library/idl/log
    voicetech/library/itags
)

END()

RECURSE_FOR_TESTS(ut)
