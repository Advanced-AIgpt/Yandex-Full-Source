LIBRARY()

OWNER(
    g:alice_quality
    smirnovpavel
)

PEERDIR(
    alice/megamind/protos/common
    alice/nlu/granet/lib/sample
    alice/nlu/libs/phrase_matching
    alice/nlu/libs/token_aligner
)

SRCS(
    frame_filler.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
