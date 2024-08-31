LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    yabio.cpp
    yabio_client.cpp
    yabio_callbacks_with_eventlog.cpp
    service.cpp
    unistat.cpp
)

RESOURCE(
    default_config.json /yabio-adapter/default_config.json
)

PEERDIR(
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/proto_configs
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/yabio/base

    voicetech/library/common
    voicetech/library/idl/log
    voicetech/library/proto_api
    voicetech/library/protobuf_handler
    voicetech/library/ws_server

    apphost/api/service/cpp

    contrib/libs/protobuf

    library/cpp/neh
    library/cpp/proto_config
    library/cpp/threading/atomic
)

END()
