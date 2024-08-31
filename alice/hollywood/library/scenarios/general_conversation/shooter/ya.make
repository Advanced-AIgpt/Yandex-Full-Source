PY3TEST()

OWNER(
    g:alice_boltalka
    deemonasd
    g:hollywood
)

SIZE(LARGE)

TAG(ya:fat)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/python/testing/it2
    alice/hollywood/library/scenarios/general_conversation/proto
)

TEST_SRCS(
    shooter.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/general_conversation/shooter/data
)

END()
