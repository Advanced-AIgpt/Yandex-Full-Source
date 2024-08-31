PY3_PROGRAM()

OWNER(g:lipkin)

PEERDIR(
    jupytercloud/tools/lib
    jupytercloud/backend/lib/clients
    contrib/python/aiohttp
    contrib/python/more-itertools
    contrib/python/tqdm
)

PY_SRCS(
    MAIN main.py
)

END()
