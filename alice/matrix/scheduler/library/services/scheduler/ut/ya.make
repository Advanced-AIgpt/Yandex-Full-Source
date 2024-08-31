GTEST()

OWNER(
    g:matrix
)

SRCS(
    utils_ut.cpp
)

PEERDIR(
    alice/matrix/scheduler/library/services/scheduler

    library/cpp/testing/gtest
    library/cpp/testing/gtest_protobuf
)

END()
