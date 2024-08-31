PROGRAM()

OWNER(
    krom
    g:alice_boltalka
)

SRCS(
    main.cpp
)

PEERDIR(
    mapreduce/yt/client
    mapreduce/yt/interface
    library/cpp/getopt/small
    alice/boltalka/extsearch/base/search
)

ALLOCATOR(LF)

END()
