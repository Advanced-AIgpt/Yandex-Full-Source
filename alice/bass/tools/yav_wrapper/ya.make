OWNER(g:bass)

PY2_PROGRAM(yav_wrapper)

USE_PYTHON2()

PEERDIR(
    library/python/vault_client
)

PY_SRCS(
    __main__.py
)

END()
