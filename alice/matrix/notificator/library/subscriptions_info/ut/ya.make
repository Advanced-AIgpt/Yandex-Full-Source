GTEST()

OWNER(
    g:matrix
)

SRCS(
    subscriptions_info_ut.cpp
)

PEERDIR(
    alice/matrix/notificator/library/subscriptions_info

    library/cpp/testing/gtest
)

END()
