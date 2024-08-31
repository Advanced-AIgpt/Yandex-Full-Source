UNITTEST_FOR(alice/nlu/libs/contacts)

OWNER(
    deemonasd
    g:alice_quality
)

SRCS(
    contacts_custom_entity_ut.cpp
    contacts_granet_ut.cpp
)

PEERDIR(
    alice/library/proto
    alice/nlu/libs/entity_searcher
    library/cpp/testing/unittest
)

END()
