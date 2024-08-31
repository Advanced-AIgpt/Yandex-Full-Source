LIBRARY()

OWNER(g:voicetech-infra)

GENERATE_ENUM_SERIALIZATION(cachalot_client.h)

SRCS(
    cachalot_client.cpp
    metrics.cpp
    service.cpp
    tts_cache.cpp
    tts_cache_callbacks_with_eventlog.cpp
)

RESOURCE(
    default_config.json /tts_cache_proxy/default_config.json
)

PEERDIR(
    alice/cachalot/api/protos
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/metrics
    alice/cuttlefish/library/proto_configs
    alice/cuttlefish/library/tts/cache/base

    voicetech/library/ws_server

    apphost/api/service/cpp

    library/cpp/neh
    library/cpp/threading/atomic
)

END()
