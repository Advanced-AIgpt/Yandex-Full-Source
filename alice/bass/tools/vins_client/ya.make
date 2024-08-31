PROGRAM()

OWNER(g:bass)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/getopt
    library/cpp/neh
    library/cpp/scheme
    library/cpp/streams/factory
    library/cpp/threading/blocking_queue
    library/cpp/threading/future
    alice/bass/libs/fetcher
    alice/library/network
)

END()
