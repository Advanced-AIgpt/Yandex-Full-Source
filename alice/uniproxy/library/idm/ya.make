PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    role_manager.py
    simple_role_manager.py
    tornado_handlers.py
    ydb_role_manager.py
)

PEERDIR(
    alice/uniproxy/library/auth
    contrib/python/tornado/tornado-4
    ydb/public/sdk/python
)

END()
