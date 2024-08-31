LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    log_context.cpp
    event_log.cpp
)

PEERDIR(
    alice/matrix/library/logging/events
    alice/matrix/library/rtlog

    alice/cuttlefish/library/logging
)

END()

RECURSE(
    events
)
