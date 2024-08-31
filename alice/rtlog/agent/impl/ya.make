LIBRARY()

OWNER(
    gusev-p
    g:kwyt
)

SRCS(
    counters.cpp
    message_sender.cpp
    rtlog_handler.cpp
    server.cpp
)

PEERDIR(
    alice/rtlog/agent/protos
    alice/rtlog/common/eventlog
    robot/rthub/misc
    library/cpp/eventlog
)


   YQL_LAST_ABI_VERSION()


END()
