PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    rtc.py
    qloud.py
    yp.py
)

PEERDIR(
    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/settings
    library/python/vault_client
)

END()

RECURSE_FOR_TESTS(ut)
