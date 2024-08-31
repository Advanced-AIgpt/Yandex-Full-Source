LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    main.cpp
)

PEERDIR(
    alice/matrix/library/service_runner

    voicetech/library/evlogdump

    library/cpp/getopt
)

END()
