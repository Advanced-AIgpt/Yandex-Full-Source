PROGRAM()

OWNER(g:paskills)

PEERDIR(
    alice/paskills/granet_server/library
    alice/paskills/granet_server/config/proto
    contrib/libs/protobuf
    kernel/server
    library/cpp/getopt
)

SRCS(
    main.cpp
)

END()
