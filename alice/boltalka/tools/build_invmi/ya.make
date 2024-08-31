PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    main.cpp
)

PEERDIR(
    alice/boltalka/libs/invmi
    library/cpp/getopt/small
    util
)

ALLOCATOR(LF)

END()
