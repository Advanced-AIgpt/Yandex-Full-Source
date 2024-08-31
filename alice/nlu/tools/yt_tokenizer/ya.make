PROGRAM()

OWNER(
    vl-trifonov
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/tokenization
    library/cpp/getopt
    mapreduce/yt/client
)

SRCS(
    yt_tokenizer.cpp
)

END()
