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

    alice/megamind/protos/scenarios
)

END()

RECURSE_FOR_TESTS(
    ut
)
