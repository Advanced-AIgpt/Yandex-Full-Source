PY3TEST()

OWNER(
    g:voicetech-infra
)


TEST_SRCS(
    client_ut.py
    client_mock_ut.py
    pool_ut.py
)

PEERDIR(
    alice/uniproxy/library/unimemcached
    alice/uniproxy/library/testing
)

END()
