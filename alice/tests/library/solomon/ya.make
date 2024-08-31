PY3_LIBRARY()

OWNER(mihajlova)

PY_SRCS(
    __init__.py
)

PEERDIR(
    alice/tests/library/vault
    alice/tests/library/ydb
    contrib/python/requests
    library/python/monlib
)

END()
