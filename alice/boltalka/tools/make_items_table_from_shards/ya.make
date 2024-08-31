PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    main.cpp
)

PEERDIR(
    mapreduce/yt/client
    mapreduce/yt/interface
    library/cpp/getopt/small
    util
)

ALLOCATOR(LF)

END()
