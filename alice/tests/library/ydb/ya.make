PY3_LIBRARY()

OWNER(mihajlova)

PY_SRCS(
    __init__.py
)

PEERDIR(
    alice/tests/library/vault
    ydb/public/sdk/python
)

END()
