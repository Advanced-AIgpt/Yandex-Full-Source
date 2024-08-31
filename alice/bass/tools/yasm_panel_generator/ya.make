PROGRAM()

OWNER(g:bass)

SRCS(
    main.cpp
)

PEERDIR(
    alice/bass/libs/config
    alice/bass/libs/metrics
    alice/bass/libs/source_request
    library/cpp/getopt/small
    library/cpp/neh
    library/cpp/scheme
)

END()
