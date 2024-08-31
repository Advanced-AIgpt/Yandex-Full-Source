PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    rtc_ut.py
    yp_ut.py
)

PEERDIR(
    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/resolvers
    alice/uniproxy/library/settings
    alice/uniproxy/library/testing
)

END()
