GTEST()

OWNER(
    g:matrix
)

SRCS(
    utils_ut.cpp
)

PEERDIR(
    alice/matrix/worker/library/services/worker

    library/cpp/testing/gtest
)

END()
