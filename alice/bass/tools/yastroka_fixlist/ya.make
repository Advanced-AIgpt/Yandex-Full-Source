PROGRAM(mktrie)

OWNER(imakunin)

PEERDIR(
    library/cpp/getopt
    library/cpp/containers/comptrie
    mapreduce/yt/client
)

SRCS(
    main.cpp
)

END()

RECURSE(
    loader
)
