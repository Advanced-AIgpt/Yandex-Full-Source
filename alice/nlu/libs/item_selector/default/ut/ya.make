UNITTEST_FOR(alice/nlu/libs/item_selector/default)

OWNER(
    volobuev
    g:alice_quality
)

DEPENDS(
    alice/nlu/data/ru/boltalka_dssm
)

DATA(
    arcadia/alice/nlu/libs/item_selector/default/ut
    arcadia/alice/nlu/libs/item_selector/default/data
)

PEERDIR(
    library/cpp/testing/unittest
)

SRCS(
    default_ut.cpp
)

END()
