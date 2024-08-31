PROGRAM()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/music_match/base
    alice/cuttlefish/library/service_runner
    library/cpp/getopt
)

RESOURCE(
    default_config.json /music_match_fake_server/default_config.json
)

SRCS(
    main.cpp
)

END()
