PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    auth_error.py
    auth.py
    client.py
    client_entry_base.py
    client_locator.py
    exception.py
    msgsettings.py
)

PEERDIR(
    alice/uniproxy/library/auth
    alice/uniproxy/library/backends_common
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/settings
    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/logging
    alice/uniproxy/library/protos
    alice/uniproxy/library/utils
    alice/uniproxy/library/backends_memcached
    ydb/public/sdk/python
    library/python/cityhash
)

END()

RECURSE_FOR_TESTS(ut)
