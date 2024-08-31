LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    tts_cache_client.cpp
)

PEERDIR(
    alice/cuttlefish/library/api
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/proto_configs
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/tts/cache/base

    apphost/api/client
    apphost/lib/compression
    apphost/lib/grpc/client

    library/cpp/proto_config
)

END()
