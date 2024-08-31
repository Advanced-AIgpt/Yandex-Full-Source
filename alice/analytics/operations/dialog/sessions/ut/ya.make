PY3TEST()

OWNER(
    irinfox
    g:alice_analytics
)

PEERDIR(
    alice/analytics/operations/dialog/sessions
)

TEST_SRCS(
    test_intent_scenario_mapping.py
)

END()
