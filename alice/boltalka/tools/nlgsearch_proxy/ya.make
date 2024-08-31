PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/getopt/small
    library/cpp/json
    library/cpp/neh
    kernel/server
    library/cpp/cgiparam
)

ALLOCATOR(LF)

END()
