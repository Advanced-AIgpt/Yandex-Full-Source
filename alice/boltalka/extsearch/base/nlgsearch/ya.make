PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

PEERDIR(
    alice/boltalka/extsearch/base/search
    search/daemons/extbasesearch
    search/daemons/httpsearch
    search/factory/basesearch/extsearch
)

SRCS(
    main.cpp
)

ALLOCATOR(LF)

END()

