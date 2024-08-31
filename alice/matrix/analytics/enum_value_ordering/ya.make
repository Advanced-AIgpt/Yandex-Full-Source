LIBRARY()

OWNER(
    g:matrix
)

PEERDIR(
    alice/matrix/analytics/protos
)

SRCS(
    enum_value_ordering.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
