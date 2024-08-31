PY3TEST()

OWNER(
    g:developersyandextaxi
    artfulvampire
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/taximeter/it
)

TEST_SRCS(
    runner.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/taximeter/it/data
)

REQUIREMENTS(ram:32)

END()
