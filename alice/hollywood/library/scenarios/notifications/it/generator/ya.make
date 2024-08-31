PY3TEST()

OWNER(
    tolyandex
    g:hollywood
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/generator_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/notifications/it
)

TEST_SRCS(generator.py)

DATA(arcadia/alice/hollywood/library/scenarios/notifications/it/data)

END()
