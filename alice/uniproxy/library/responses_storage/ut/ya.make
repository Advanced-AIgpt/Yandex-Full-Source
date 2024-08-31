
PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    responses_storage_ut.py
)

PEERDIR(
    alice/uniproxy/library/responses_storage
    alice/uniproxy/library/testing
)

END()
