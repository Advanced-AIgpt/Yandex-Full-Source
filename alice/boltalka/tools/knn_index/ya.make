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
    extsearch/images/quality/mr_kmeans
    library/cpp/getopt/small
    library/cpp/knn_index
    util
)

ALLOCATOR(LF)

END()
