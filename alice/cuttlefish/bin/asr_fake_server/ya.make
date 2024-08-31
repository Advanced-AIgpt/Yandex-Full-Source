PROGRAM()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/asr/base
    alice/cuttlefish/library/service_runner
    library/cpp/getopt
)

RESOURCE(
    default_config.json /asr_fake_server/default_config.json
)

SRCS(
    main.cpp
)

END()
