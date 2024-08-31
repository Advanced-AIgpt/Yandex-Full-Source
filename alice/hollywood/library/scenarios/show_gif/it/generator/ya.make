PY3TEST()

OWNER(
    jan-fazli
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/generator_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/show_gif/it
)

TEST_SRCS(generator.py)

DATA(arcadia/alice/hollywood/library/scenarios/show_gif/it/data)

END()
