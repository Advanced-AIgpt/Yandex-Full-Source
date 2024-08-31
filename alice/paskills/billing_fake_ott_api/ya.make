PY3_PROGRAM()

PEERDIR(
    contrib/python/aiohttp
    library/python/resource
    library/python/ylog
)

PY_SRCS(
    MAIN main.py
)

RESOURCE(
    resources/content_available.json content_available.json
    resources/content_options.json content_options.json
)

END()

OWNER(
    g:paskills
)
