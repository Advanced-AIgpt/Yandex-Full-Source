PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    client_locator_ut.py
    auth_ut.py
    mock_auth.py
)

PEERDIR(
    alice/uniproxy/library/auth/mocks
    alice/uniproxy/library/messenger
    alice/uniproxy/library/logging
    alice/uniproxy/library/testing
)

END()
