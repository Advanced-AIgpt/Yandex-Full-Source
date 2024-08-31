PY3_LIBRARY()

OWNER(g:voicetech-infra)

PY_SRCS(
    __init__.py
    http_methods.py
    Nanny.py
    nanny_fix_tags.py
    Puncher.py
    tools.py
    Yp.py
)

PEERDIR(
    contrib/python/aiohttp
)

END()
