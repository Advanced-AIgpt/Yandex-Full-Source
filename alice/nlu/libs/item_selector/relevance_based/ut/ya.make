UNITTEST_FOR(alice/nlu/libs/item_selector/relevance_based)

OWNER(
    volobuev
    g:alice_quality
)

PEERDIR(
    library/cpp/testing/gmock_in_unittest
    library/cpp/testing/unittest
)

SRCS(
    relevance_based_ut.cpp
    lstm_relevance_computer_ut.cpp
    ut/mock.cpp
)

END()
