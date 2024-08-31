LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    user_white_list.cpp
)

PEERDIR(
    alice/matrix/notificator/library/config
)

END()

RECURSE_FOR_TESTS(
    ut
)
