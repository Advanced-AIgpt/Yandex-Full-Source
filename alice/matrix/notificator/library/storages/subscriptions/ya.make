LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    storage.cpp
)

PEERDIR(
    alice/matrix/notificator/library/storages/utils

    alice/matrix/library/ydb

    library/cpp/digest/md5
)

END()
