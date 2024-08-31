PY3TEST()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/uniproxy/library/events
    alice/uniproxy/library/experiments
)

DATA(
    arcadia/alice/uniproxy/experiments
    arcadia/alice/uniproxy/library/experiments/ut
)

DEPENDS(
    alice/uniproxy/experiments
)

TEST_SRCS(
    test_experiments.py
    test_config_validation.py
)

END()
