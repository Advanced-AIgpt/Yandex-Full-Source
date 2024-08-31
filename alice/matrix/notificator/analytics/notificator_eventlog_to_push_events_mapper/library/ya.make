LIBRARY()

OWNER(
    g:matrix
)

PEERDIR(
    alice/matrix/notificator/analytics/common

    alice/matrix/library/logging/events

    library/cpp/eventlog
)

SRCS(
    event_processor.cpp
    mapper.cpp
    table_helper.cpp
)

END()
