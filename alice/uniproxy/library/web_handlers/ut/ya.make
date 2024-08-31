PY3TEST()
OWNER(g:voicetech-infra)

PEERDIR(
    contrib/python/tornado/tornado-4
    alice/uniproxy/library/logging
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/web_handlers
    alice/uniproxy/library/extlog/mocks
    alice/uniproxy/library/auth/mocks
    alice/uniproxy/library/testing
    voicetech/library/proto_api
)

DEPENDS(
    voicetech/infra/tools/websocketproxy-recognizer
)

TEST_SRCS(
    common.py
    test_post_asr_handler.py
    test_asr_web_socket.py
)

SIZE(MEDIUM)
TIMEOUT(180)

END()
