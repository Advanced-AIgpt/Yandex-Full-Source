PROGRAM()

OWNER(
    kolyakolya
    g:alice_quality
)

PEERDIR(
    library/cpp/getopt
    library/cpp/logger/global
    mapreduce/yt/interface
    mapreduce/yt/library/operation_tracker
    mapreduce/yt/util
    mapreduce/yt/client
    util/draft
)

SRCS(
    collect_learn.cpp
    main.cpp
)

END()
