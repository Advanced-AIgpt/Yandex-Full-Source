LIBRARY()

OWNER(gusev-p)

SRCS(
    client.cpp
    common.cpp
)

IF (RTLOG_AGENT)
    PEERDIR(
        robot/rthub/misc
    )
ENDIF()

PEERDIR(
    alice/rtlog/protos
    logbroker/unified_agent/client/cpp/logger
    robot/library/fork_subscriber
    library/cpp/eventlog
    contrib/libs/protobuf
)

END()

RECURSE(
    python
    shared_lib
)
