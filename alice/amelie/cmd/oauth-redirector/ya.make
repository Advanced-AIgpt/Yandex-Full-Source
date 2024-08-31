PY3_PROGRAM(oauth-redirector)

OWNER(alkapov)

PEERDIR(
    contrib/python/aiohttp
    contrib/python/click
)

PY_SRCS(
    MAIN main.py
)

END()
