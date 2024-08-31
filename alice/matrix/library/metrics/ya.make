LIBRARY()

OWNER(
    g:matrix
)

PEERDIR(
    infra/libs/sensors

    library/cpp/http/misc
)

SRCS(
    metrics.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
