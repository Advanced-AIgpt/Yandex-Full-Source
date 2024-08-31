LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    config.h
    config.cpp
)

PEERDIR(
    alice/cuttlefish/library/proto_configs

    library/cpp/proto_config
    library/cpp/resource
)

RESOURCE(
    default_config.json /cuttlefish/proto_config/config.json
)

END()

RECURSE_FOR_TESTS(ut)
