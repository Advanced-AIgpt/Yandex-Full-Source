PY3TEST()
OWNER(
    zubchick
    g:megamind
)

PEERDIR(
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/protos/extensions
)

SIZE(SMALL)

TEST_SRCS(
    test_device_state.py
    test_directives.py
)

END()
