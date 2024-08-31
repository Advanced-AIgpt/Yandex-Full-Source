PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    http_client_ut.py
)

PEERDIR(
    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/testing
)

DATA(
    arcadia/alice/uniproxy/library/async_http_client/ut/uniproxy_ca.crt
    arcadia/alice/uniproxy/library/async_http_client/ut/uniproxy.crt
    arcadia/alice/uniproxy/library/async_http_client/ut/uniproxy.key
)

END()
