PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    locator.py
    notificator_api.py
    metrics.py
)

PEERDIR(
    alice/rtlog/client/python/lib

    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/protos/api/matrix
    alice/protos/api/notificator

    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/protos
    alice/uniproxy/library/logging
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/settings
    alice/uniproxy/library/backends_common

    contrib/python/tornado/tornado-4
)

END()


RECURSE_FOR_TESTS(ut)
