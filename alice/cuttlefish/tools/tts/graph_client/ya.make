PROGRAM(tts-client)

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/protos

    apphost/lib/grpc/client
    apphost/lib/compression
    apphost/api/client

    library/cpp/getopt
)

SRCS(
    main.cpp
)

END()
