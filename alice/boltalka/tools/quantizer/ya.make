PROGRAM()

OWNER(
    krom
    g:alice_boltalka
)

PEERDIR(
    mapreduce/yt/client
    mapreduce/yt/interface
    mapreduce/yt/util
    util
    library/cpp/getopt/small
    library/cpp/dot_product
    library/cpp/threading/local_executor
)

SRCS(
    main.cpp
    quantizer.cpp
    borders_retrieval.cpp
    metrics.cpp
)

END()
