PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/containers/heap_dict
    library/cpp/getopt/small
    util
)

ALLOCATOR(LF)

END()
