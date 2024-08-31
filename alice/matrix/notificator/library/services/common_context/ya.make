LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    common_context.cpp
)

PEERDIR(
    alice/matrix/notificator/library/config

    alice/matrix/library/rtlog
    alice/matrix/library/ydb
)

END()
