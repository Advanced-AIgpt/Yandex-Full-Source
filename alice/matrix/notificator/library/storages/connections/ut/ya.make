GTEST()

OWNER(
    g:matrix
)

SRCS(
    storage_ut.cpp
)

PEERDIR(
    alice/matrix/notificator/library/storages/connections

    library/cpp/testing/gtest
)

END()