GTEST()

OWNER(
    g:matrix
)

SRCS(
    client_ut.cpp
)

PEERDIR(
    alice/matrix/notificator/library/pushes_and_notifications

    library/cpp/testing/gtest
)

END()
