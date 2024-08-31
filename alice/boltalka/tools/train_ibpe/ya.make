RECURSE(
    ibpe_test
)

PROGRAM()

OWNER(
    alipov
    g:alice_boltalka
)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/containers/dense_hash
    library/cpp/containers/heap_dict
    library/cpp/getopt/small
    util
)

ALLOCATOR(LF)

END()
