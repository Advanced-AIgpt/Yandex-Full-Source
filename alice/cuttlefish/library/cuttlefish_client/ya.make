LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    cuttlefish_client.cpp
)

PEERDIR(
    alice/cuttlefish/library/proto_configs
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/asr/base
    alice/cuttlefish/library/tts/backend/base

    apphost/lib/grpc/client
    apphost/lib/compression
    apphost/api/client

    contrib/libs/protobuf

    library/cpp/proto_config
)

END()
