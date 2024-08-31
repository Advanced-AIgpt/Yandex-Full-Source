LIBRARY()

OWNER(
    g:matrix
)

PEERDIR(
    infra/libs/outcome
)

SRCS(
    utils.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
