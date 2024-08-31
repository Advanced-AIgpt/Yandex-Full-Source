LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    music_match_client.cpp
)

PEERDIR(
    alice/cuttlefish/library/api
    alice/cuttlefish/library/music_match/base
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
