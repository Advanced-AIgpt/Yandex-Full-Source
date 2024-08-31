UNITTEST_FOR(alice/hollywood/library/scenarios/food)

OWNER(
    samoylovboris
    the0
    g:alice_quality
)

PEERDIR(
    alice/hollywood/library/scenarios/food/nlg
    alice/nlg/library/nlg_renderer
    alice/nlg/library/testing
    alice/nlu/granet/lib/utils
)

SRCS(
    dialog_config_ut.cpp
)

REQUIREMENTS(ram:9)

END()
