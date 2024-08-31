PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/cgiparam
    library/cpp/getopt/small
    library/cpp/http/simple
    util
)

ALLOCATOR(LF)

END()
