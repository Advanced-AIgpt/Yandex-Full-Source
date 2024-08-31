LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    apphost_dlog.cpp
    apphost_log.cpp
    dlog.cpp
    event_log.cpp
    log_context.cpp
)

PEERDIR(
    alice/cuttlefish/library/rtlog

    voicetech/library/logger
)

END()
