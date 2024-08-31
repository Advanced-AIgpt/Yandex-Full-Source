PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    personal_data_ut.py
)

PEERDIR(
    alice/rtlog/client/python/lib

    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/auth
    alice/uniproxy/library/auth/mocks
    alice/uniproxy/library/logging
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/personal_data
    alice/uniproxy/library/settings
    alice/uniproxy/library/testing
    alice/uniproxy/library/utils

    contrib/python/tornado/tornado-4
)

END()
