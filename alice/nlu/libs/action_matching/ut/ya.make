UNITTEST_FOR(alice/nlu/libs/action_matching)

OWNER(
    smirnovpavel
    g:alice_quality
)

DEPENDS(
    alice/nlu/data/ru/boltalka_dssm
)

PEERDIR(
    alice/library/frame
    alice/library/unittest
    alice/nlu/libs/item_selector/testing
)

SRCS(
    action_matching_ut.cpp
)

END()
