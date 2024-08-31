PROGRAM()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/asr/adapter
    alice/cuttlefish/library/service_runner
    library/cpp/getopt
)

SRCS(
    main.cpp
)

END()

RECURSE_FOR_TESTS(ut)