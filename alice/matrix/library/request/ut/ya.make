GTEST()

OWNER(
    g:matrix
)

SRCS(
    http_request_ut.cpp
)

PEERDIR(
    alice/matrix/library/request

    library/cpp/testing/gtest
)

END()
