PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    global_state_ut.py
)

PEERDIR(
    alice/uniproxy/library/global_state
)

END()
