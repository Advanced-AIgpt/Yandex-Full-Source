PY3TEST()

OWNER(
    g:milab
    lvlasenkov
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/transform_face/it
    alice/hollywood/library/scenarios/transform_face/proto
)

TEST_SRCS(
    runner.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/transform_face/it/data
)

REQUIREMENTS(ram:32)

END()
