LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    application.cfgproto
    common.cfgproto
    load.cpp
    yabio_context.cfgproto
)

PEERDIR(
    library/cpp/proto_config/protos
    library/cpp/proto_config
    library/cpp/resource
)

RESOURCE(
    cachalot-activation.json /alice/cachalot/library/config/cachalot-activation.json
    cachalot-beta.json /alice/cachalot/library/config/cachalot-beta.json
    cachalot-context.json /alice/cachalot/library/config/cachalot-context.json
    cachalot-gdpr.json /alice/cachalot/library/config/cachalot-gdpr.json
    cachalot-mm.json /alice/cachalot/library/config/cachalot-mm.json
    cachalot-tts.json /alice/cachalot/library/config/cachalot-tts.json
)

END()

RECURSE_FOR_TESTS(
    ut
)
