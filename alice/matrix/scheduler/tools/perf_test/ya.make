PROGRAM(perf-test)

OWNER(
    g:matrix
)

PEERDIR(
    alice/protos/api/matrix

    apphost/lib/grpc/client
    apphost/lib/compression
    apphost/api/client

    library/cpp/getopt
    library/cpp/protobuf/interop
    library/cpp/threading/future
)

SRCS(
    main.cpp
)

END()
