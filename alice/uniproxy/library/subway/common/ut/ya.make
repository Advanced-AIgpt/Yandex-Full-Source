PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    registry_ut.py
)

PEERDIR(
    alice/uniproxy/library/subway/common
)

END()
