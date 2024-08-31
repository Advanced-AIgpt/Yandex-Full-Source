PROGRAM()

OWNER(
    deemonasd
    g:alice
)

SRCS(
    main.cpp
)

PEERDIR(
    alice/begemot/lib/fixlist_index

    library/cpp/getopt

    mapreduce/yt/client
    mapreduce/yt/interface
    mapreduce/interface
)

END()
