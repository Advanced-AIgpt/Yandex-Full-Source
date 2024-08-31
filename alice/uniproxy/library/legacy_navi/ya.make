PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    navi_adapter.py
    navi_request.py
)

PEERDIR(
    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/common_handlers
    alice/uniproxy/library/settings
    alice/uniproxy/library/global_counter

    contrib/python/attrs
    contrib/python/lxml
)

END()

RECURSE_FOR_TESTS(ut)
