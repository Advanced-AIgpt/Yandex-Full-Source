LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    asr1_client.cpp
    asr2.cpp
    asr2_client.cpp
    asr2_via_asr1_client.cpp
    asr_callbacks_with_eventlog.cpp
    protocol_convertor.cpp
    service.cpp
    unistat.cpp
)

RESOURCE(
    default_config.json /asr-adapter/default_config.json
)

PEERDIR(
    alice/cuttlefish/library/asr/base
    alice/cuttlefish/library/experiments
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/proto_censor
    alice/cuttlefish/library/proto_configs
    alice/cuttlefish/library/protos

    voicetech/library/common
    voicetech/library/idl/log
    voicetech/library/proto_api
    voicetech/library/protobuf_handler
    voicetech/library/ws_server

    apphost/api/service/cpp

    contrib/libs/protobuf
    contrib/libs/pugixml

    library/cpp/json
    library/cpp/neh
    library/cpp/proto_config
    library/cpp/threading/atomic
)

END()
