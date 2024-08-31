LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    storage.cpp
)

PEERDIR(
    alice/matrix/notificator/library/storages/connections/protos

    alice/matrix/library/ydb
)

END()

RECURSE(
    protos
)

RECURSE_FOR_TESTS(
    ut
)
