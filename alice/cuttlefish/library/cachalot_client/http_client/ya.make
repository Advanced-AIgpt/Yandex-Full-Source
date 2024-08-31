LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    cachalot_client.cpp
)

PEERDIR(
    alice/cachalot/api/protos
    alice/cuttlefish/library/proto_configs
    contrib/libs/protobuf
    library/cpp/proto_config
    library/cpp/http/client
    # voicetech/library/ws_server
    apphost/lib/grpc/client
    apphost/lib/compression
    apphost/api/client
    apphost/lib/transport
)

END()
