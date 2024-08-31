PY3TEST()

OWNER(
    g:milab
    lvlasenkov
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/happy_new_year/it
)

TEST_SRCS(
    runner.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/happy_new_year/it/data
)

REQUIREMENTS(ram:32)

END()