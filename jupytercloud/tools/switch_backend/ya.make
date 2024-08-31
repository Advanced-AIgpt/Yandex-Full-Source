PY3_PROGRAM()

OWNER(g:jupyter-cloud)

PEERDIR(
    contrib/python/click
    contrib/python/redis

    library/python/vault_client
)

PY_SRCS(
    MAIN main.py
)

END()
