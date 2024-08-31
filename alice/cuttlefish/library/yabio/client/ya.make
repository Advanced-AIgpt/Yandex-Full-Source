LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    yabio_client.cpp
)

PEERDIR(
    alice/cuttlefish/library/api
    alice/cuttlefish/library/proto_configs
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/yabio/base

    voicetech/library/ws_server

    apphost/lib/grpc/client
    apphost/lib/compression
    apphost/api/client

    contrib/libs/protobuf

    library/cpp/proto_config
)

END()
