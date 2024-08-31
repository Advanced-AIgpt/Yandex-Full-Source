LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    asr_recognize.cpp
    asr_result.cpp
    biometry.cpp
    matched_user_event_handler.cpp
    megamind.cpp
    music_match_request.cpp
    music_match_response.cpp
    proto_to_ws_stream.cpp
    service.cpp
    smart_activation.cpp
    support_functions.cpp
    tts_generate.cpp
    tts_generate_response.cpp
    vins_timings.cpp
    ws_stream_to_proto.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/stream_converter/rms_converter
    alice/cuttlefish/library/cuttlefish/context_load/client

    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/cuttlefish/tts/utils

    alice/cachalot/api/protos
    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/convert
    alice/cuttlefish/library/experiments
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/proto_censor
    alice/cuttlefish/library/proto_converters
    alice/cuttlefish/library/protos

    alice/library/json
    alice/library/proto
    alice/megamind/api/request

    voicetech/library/messages

    apphost/api/service/cpp

    library/cpp/json
    library/cpp/protobuf/interop
    library/cpp/protobuf/json
    library/cpp/resource
)

RESOURCE(
    alice/uniproxy/experiments/vins_experiments.json            /experiments/macros.json
    alice/uniproxy/experiments/experiments_rtc_production.json  /experiments/experiments.json
)

END()

RECURSE(
    rms_converter
)

RECURSE_FOR_TESTS(ut)
