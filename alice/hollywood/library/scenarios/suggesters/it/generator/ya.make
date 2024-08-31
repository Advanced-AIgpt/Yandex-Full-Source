PY3TEST()

OWNER(
    dan-anastasev
    g:hollywood
    vitvlkv
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/generator_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/suggesters/it
)

TEST_SRCS(
    generator.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/suggesters/it/data_games
    arcadia/alice/hollywood/library/scenarios/suggesters/it/data_movies
)

END()
