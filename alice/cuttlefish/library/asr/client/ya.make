LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    asr_client.cpp
)

PEERDIR(
    alice/cuttlefish/library/api
    alice/cuttlefish/library/asr/base
    alice/cuttlefish/library/proto_configs
    alice/cuttlefish/library/protos

    voicetech/library/ws_server

    apphost/lib/grpc/client
    apphost/lib/compression
    apphost/api/client

    contrib/libs/protobuf

    library/cpp/proto_config
)

END()
