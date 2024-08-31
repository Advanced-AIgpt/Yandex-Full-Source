PROGRAM()

OWNER(
    nzinov
    g:alice_boltalka
)

SRCS(generator.cpp)

PEERDIR(
    mapreduce/yt/client
    mapreduce/yt/interface
    alice/boltalka/libs/nlgsearch_simple
    library/cpp/dot_product
)

END()
