PY2TEST()

OWNER(justdev)

TEST_SRCS(test.py)

PEERDIR(
    alice/vins/apps/personal_assistant/personal_assistant/view_state/lib
)

DATA(arcadia/alice/vins/apps/personal_assistant/personal_assistant/view_state/tests/data/view_state.json)

END()
