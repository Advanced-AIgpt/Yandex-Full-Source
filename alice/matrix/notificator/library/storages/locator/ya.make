LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    storage.cpp
)

PEERDIR(
    alice/matrix/notificator/library/storages/connections
    alice/matrix/notificator/library/storages/utils

    alice/matrix/library/ydb

    alice/uniproxy/library/protos
)

END()
