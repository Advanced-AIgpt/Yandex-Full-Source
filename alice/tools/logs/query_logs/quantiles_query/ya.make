PROGRAM()

OWNER(g:megamind)

PEERDIR(
    library/cpp/getopt
    mapreduce/yt/client
    mapreduce/yt/util
    alice/library/logs/uniproxy
    library/cpp/cgiparam
)

SRCS(quantile_reqids.cpp)

END()

