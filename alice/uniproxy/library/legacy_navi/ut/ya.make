PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    navi_adapter_ut.py
)

PEERDIR(
    alice/uniproxy/library/legacy_navi
)

END()
