PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/dot_product
    library/cpp/getopt/small
    library/cpp/threading/local_executor
    util
)

ALLOCATOR(LF)

END()
