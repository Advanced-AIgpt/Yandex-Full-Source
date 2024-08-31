UNITTEST_FOR(alice/begemot/lib/frame_filler)

OWNER(
    smirnovpavel
    g:alice_quality
)

PEERDIR(
    alice/begemot/lib/frame_filler
    alice/nlu/libs/token_aligner
    search/begemot/rules/external_markup/proto
    search/begemot/rules/occurrences/custom_entities/rule/proto
)

SRCS(
    frame_filler_ut.cpp
)

END()
