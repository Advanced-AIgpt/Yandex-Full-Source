PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    event_ut.py
    extra_ut.py
    directive_ut.py
    streamcontrol_ut.py
)

PEERDIR(
    alice/uniproxy/library/events
    alice/uniproxy/library/utils
)

END()
