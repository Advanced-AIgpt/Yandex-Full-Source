PROGRAM()

OWNER(
    g:alice_boltalka
)

SRCS(
    main.cpp
)

PEERDIR(
    alice/boltalka/extsearch/base/search
    library/cpp/getopt/small
    util
)

ALLOCATOR(LF)

END()
