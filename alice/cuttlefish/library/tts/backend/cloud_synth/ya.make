LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    cloud_synth.cpp
    service.cpp
)

RESOURCE(
    default_config.json /cloud_synth/default_config.json
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common

    alice/cuttlefish/library/logging
    alice/cuttlefish/library/metrics
    alice/cuttlefish/library/proto_censor
    alice/cuttlefish/library/proto_configs
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/tts/backend/base

    voicetech/asr/cloud_engine/api/speechkit/v3/service/tts
    voicetech/library/common
    voicetech/library/idl/log
    voicetech/library/proto_api
    voicetech/library/protobuf_handler
    voicetech/library/ws_server

    apphost/api/service/cpp

    library/cpp/json
    library/cpp/neh
)

END()
