PROGRAM()

OWNER(g:megamind)

PEERDIR(
    alice/bass/libs/logging
    library/cpp/getopt
    library/cpp/threading/future
    mapreduce/yt/client
    mapreduce/yt/util
)

SRCS(main.cpp)

END()
