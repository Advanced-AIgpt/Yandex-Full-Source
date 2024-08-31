PY3TEST()

OWNER(
    akastornov
    g:hollywood
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/generator_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/messenger_call/it
)

TEST_SRCS(
    generator.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/messenger_call/it/data
)

END()
