PROGRAM()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/tts/backend/base
    alice/cuttlefish/library/service_runner

    library/cpp/getopt
)

RESOURCE(
    default_config.json /tts_fake_server/default_config.json
)

SRCS(
    main.cpp
)

END()
