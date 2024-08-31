PY3TEST()

OWNER(
    g:voicetech-infra
    zhigan
)

TEST_SRCS(
    personal_cards_ut.py
)

PEERDIR(
    alice/rtlog/client/python/lib

    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/logging
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/personal_cards
    alice/uniproxy/library/settings
    alice/uniproxy/library/testing
    alice/uniproxy/library/utils

    contrib/python/tornado/tornado-4
)

END()
