PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    origin_restrictions_ut.py
    test_uniwebsocket.py
)

PEERDIR(
    alice/uniproxy/library/auth/mocks
    alice/uniproxy/library/extlog
    alice/uniproxy/library/extlog/mocks
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/global_state
    alice/uniproxy/library/testing
    alice/uniproxy/library/unisystem
)

END()
