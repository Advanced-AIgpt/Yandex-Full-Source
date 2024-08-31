LIBRARY()

OWNER(paxakor)

NEED_REVIEW()

SRCS(
    config.proto

    app.cpp
    app_config.cpp

    subsystem_admin.cpp
    subsystem_logging.cpp
    subsystem_metrics.cpp
    subsystem_servant.cpp

    frame_converter.cpp
    http_init.cpp
    http_output.cpp
    http_processor.cpp
    input.cpp
    mmsetup.cpp
    mm_rpc_output.cpp
    mm_rpc_setup.cpp
    output.cpp

    metadata.cpp
    request_meta_setup.cpp
)

PEERDIR(
    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/cuttlefish/context_save/client
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/metrics
    alice/cuttlefish/library/protos
    alice/gproxy/library/events
    alice/gproxy/library/gsetup_conv
    alice/gproxy/library/protos
    alice/library/json
    alice/library/logger/proto
    alice/megamind/protos/scenarios
    apphost/api/client
    apphost/lib/proto_answers
    contrib/libs/protobuf
    library/cpp/getoptpb
    library/cpp/json
    voicetech/library/idl/log
    voicetech/library/itags
)

END()

RECURSE_FOR_TESTS(ut)
