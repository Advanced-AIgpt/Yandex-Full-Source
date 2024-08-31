PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    log_ut.py
)

PEERDIR(
    alice/uniproxy/library/logging
)

END()
