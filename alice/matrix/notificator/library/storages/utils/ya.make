LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    utils.cpp
)

PEERDIR(
    alice/protos/api/matrix

    library/cpp/digest/md5
)

END()

RECURSE_FOR_TESTS(
    ut
)
