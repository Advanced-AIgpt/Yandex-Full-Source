PROGRAM(mapper)

OWNER(
    g:matrix
)

PEERDIR(
    alice/matrix/notificator/analytics/notificator_eventlog_to_push_events_mapper/library

    library/cpp/getopt

    mapreduce/yt/client
)

SRCS(
    main.cpp
)

END()
