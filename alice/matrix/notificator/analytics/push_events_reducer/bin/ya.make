PROGRAM(reducer)

OWNER(
    g:matrix
)

PEERDIR(
    alice/matrix/notificator/analytics/push_events_reducer/library

    library/cpp/getopt

    mapreduce/yt/client
)

SRCS(
    main.cpp
)

END()
