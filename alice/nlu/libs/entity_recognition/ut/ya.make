UNITTEST_FOR(alice/nlu/libs/entity_recognition)

OWNER(
    the0
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/item_selector/testing
    alice/library/frame
    alice/library/unittest
    library/cpp/testing/gmock_in_unittest
    library/cpp/testing/unittest
)

SRCS(
    entity_recognition_ut.cpp
)

END()
