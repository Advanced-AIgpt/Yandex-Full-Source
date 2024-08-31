GTEST()

OWNER(
    g:matrix
)

SRCS(
    user_white_list_ut.cpp
)

PEERDIR(
    alice/matrix/notificator/library/user_white_list

    library/cpp/testing/gtest
)

END()
