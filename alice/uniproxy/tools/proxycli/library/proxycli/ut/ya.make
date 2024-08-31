PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    context_ut.py
)

PEERDIR(
    alice/uniproxy/tools/proxycli/library/proxycli
    alice/uniproxy/tools/proxycli/library/scenarios
)

RESOURCE(
    test_scenario.yaml      /scenarios/test_scenario.yaml
    test_scenario2.yaml     /scenarios/test_scenario2.yaml
)

END()
