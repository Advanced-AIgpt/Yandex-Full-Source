PY3TEST()

OWNER(
    g:marketinalice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/generator_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/market/how_much/it
)

TEST_SRCS(
    generator.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/market/how_much/it/data
)

END()
