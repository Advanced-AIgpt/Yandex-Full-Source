PY3TEST()

OWNER(
    g:vh
    antonfn
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/mordovia_video_selection/it
)

TEST_SRCS(
    runner.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/mordovia_video_selection/it/data
)

REQUIREMENTS(ram:32)

END()
