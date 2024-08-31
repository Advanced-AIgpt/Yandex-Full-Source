PY3TEST()

OWNER(
    polushkin
    g:cv-dev
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/image_what_is_this/it
)

TEST_SRCS(
    runner.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/image_what_is_this/it/data
)

REQUIREMENTS(ram:32)

END()
