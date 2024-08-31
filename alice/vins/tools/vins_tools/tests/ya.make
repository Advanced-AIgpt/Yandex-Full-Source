PY2TEST()

OWNER(
    g:alice_quality
    ddale
)

SIZE(MEDIUM)

PEERDIR(
    alice/vins/apps/personal_assistant
    alice/vins/tools/vins_tools
)


DEPENDS(
    alice/vins/resources
)

TEST_SRCS(
    nlu/inspection/test_inspection.py
)

REQUIREMENTS(ram:13)

END()
