PY3_PROGRAM()

OWNER(g:jupyter-cloud)

PEERDIR(
    contrib/python/aiohttp
    contrib/python/click
    contrib/python/asyncio-pool
    contrib/python/tqdm
    contrib/python/PyYAML

    library/python/statface_client

    jupytercloud/tools/lib
)

PY_SRCS(
    MAIN main.py
)

RESOURCE_FILES(
    blacklist.yaml
)

END()
