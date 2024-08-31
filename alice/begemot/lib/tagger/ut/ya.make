UNITTEST_FOR(alice/begemot/lib/tagger)

OWNER(
    the0
    g:alice_quality
)

PEERDIR(
    alice/begemot/lib/utils
    alice/library/frame
    alice/library/json
    alice/library/proto
    alice/library/unittest
    alice/megamind/protos/common
    library/cpp/iterator
    search/begemot/rules/granet_config/proto
    search/begemot/rules/alice/tagger/proto
    search/begemot/rules/occurrences/custom_entities/rule/proto
)

SRCS(
    tagger_slot_matching_ut.cpp
)

DEPENDS(
    alice/begemot/lib/tagger/ut/data
    alice/nlu/data/ru/granet
)

END()
