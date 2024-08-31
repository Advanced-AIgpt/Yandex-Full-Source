PY3_PROGRAM(mass_spawn)

OWNER(g:jupyter-cloud)

PEERDIR(
    jupytercloud/backend
    library/python/vault_client
)

PY_SRCS(
    __main__.py
)

END()

