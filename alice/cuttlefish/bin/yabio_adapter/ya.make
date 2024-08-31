PROGRAM()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cachalot/api/protos
    alice/cuttlefish/library/service_runner
    alice/cuttlefish/library/yabio/adapter

    library/cpp/getopt
)

SRCS(
    main.cpp
)

END()

RECURSE_FOR_TESTS(ut)
