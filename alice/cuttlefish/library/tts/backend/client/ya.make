LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    tts_client.cpp
)

PEERDIR(
    alice/cuttlefish/library/api
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/proto_configs
    alice/cuttlefish/library/protos

    voicetech/library/ws_server

    apphost/api/client
    apphost/lib/compression
    apphost/lib/grpc/client

    contrib/libs/protobuf

    library/cpp/proto_config
)

END()
