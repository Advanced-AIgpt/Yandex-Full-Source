PY3TEST()

OWNER(g:voicetech-infra)

TEST_SRCS(
    test_notificator_api.py
    test_metrics.py
)

PEERDIR(
    alice/uniproxy/library/logging
    alice/uniproxy/library/notificator_api
    alice/uniproxy/library/testing
    alice/uniproxy/library/protos
    contrib/python/tornado/tornado-4
)

END()
