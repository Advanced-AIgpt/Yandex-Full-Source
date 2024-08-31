LIBRARY()

OWNER(
    the0
    g:alice_quality
)

PEERDIR(
    alice/library/frame
    alice/megamind/protos/common
    alice/nlu/libs/frame
    alice/nlu/libs/item_selector/interface
)

SRCS(
    entity_recognition.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
