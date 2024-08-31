PY3_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/hollywood/scripts/graph_generator/library
    alice/library/python/testing/auth
    alice/library/python/utils
    library/python/vault_client
)

PY_SRCS(
    __init__.py
    apphost.py
)

END()
