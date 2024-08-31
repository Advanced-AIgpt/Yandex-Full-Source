PROGRAM()

OWNER(g:megamind)

PEERDIR(
    alice/bass/libs/client
    alice/library/logs/uniproxy
    library/cpp/cgiparam
    library/cpp/getopt
    mapreduce/yt/client
    mapreduce/yt/util
)

SRCS(durations_reqids.cpp)

END()
