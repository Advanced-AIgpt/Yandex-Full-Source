PROGRAM()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/service_runner
    alice/cuttlefish/library/yabio/base

    library/cpp/getopt
)

RESOURCE(
    default_config.json /yabio_fake_server/default_config.json
)

SRCS(
    main.cpp
)

END()
