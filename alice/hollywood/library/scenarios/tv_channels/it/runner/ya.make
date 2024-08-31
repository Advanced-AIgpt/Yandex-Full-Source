PY3TEST()

OWNER(
    g:hollywood
    vitvlkv
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/tv_channels/it
)

TEST_SRCS(
    runner.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/tv_channels/it/data
)

REQUIREMENTS(ram:32)

END()
