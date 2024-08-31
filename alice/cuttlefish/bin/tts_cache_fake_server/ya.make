PROGRAM()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/tts/cache/base
    alice/cuttlefish/library/service_runner
)

RESOURCE(
    default_config.json /tts_cache_fake_server/default_config.json
)

SRCS(
    main.cpp
)

END()
