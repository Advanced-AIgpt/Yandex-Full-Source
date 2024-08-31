GTEST()

OWNER(
    g:matrix
)

SRCS(
    enum_value_ordering_validation.cpp
)

PEERDIR(
    alice/matrix/notificator/analytics/push_events_reducer/library

    library/cpp/testing/gtest
)

END()