PROGRAM(cuttlefish)
OWNER(g:voicetech-infra)

PEERDIR(
    alice/cachalot/api/protos
    alice/cuttlefish/library/cuttlefish
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/protos

    voicetech/library/evlogdump

    library/cpp/getopt
    library/cpp/terminate_handler
)

SRCS(
    main.cpp
)

END()

RECURSE_FOR_TESTS(tests)
