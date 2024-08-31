GTEST()

OWNER(
    g:matrix
)

SRCS(
    enum_value_ordering_ut.cpp
)

PEERDIR(
    alice/matrix/analytics/enum_value_ordering
    alice/matrix/analytics/enum_value_ordering/ut/protos

    library/cpp/testing/gtest
)

END()

RECURSE(
    protos
)