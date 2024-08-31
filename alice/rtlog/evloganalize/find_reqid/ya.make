PROGRAM()

OWNER(g:alice)

PEERDIR(
    alice/rtlog/protos
    library/cpp/eventlog/dumper
    library/cpp/getopt
)

SRCS(
    main.cpp
)

END()
