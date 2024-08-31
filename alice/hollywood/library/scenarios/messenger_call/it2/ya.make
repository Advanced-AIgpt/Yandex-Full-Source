PY3TEST()

OWNER(
    flimsywhimsy
    g:alice_quality
)

SIZE(MEDIUM)

FORK_SUBTESTS()
SPLIT_FACTOR(2)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

PEERDIR(
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/hollywood/library/scenarios/messenger_call/proto
)

PY_SRCS(
    predefined_contacts.py
)

TEST_SRCS(
    tests_phone_call.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/messenger_call/it2/tests_phone_call
)

END()
