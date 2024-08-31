PY3TEST()

OWNER(
    jan-fazli
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/show_gif/it
)

TEST_SRCS(runner.py)

DATA(arcadia/alice/hollywood/library/scenarios/show_gif/it/data)

REQUIREMENTS(ram:32)

END()
