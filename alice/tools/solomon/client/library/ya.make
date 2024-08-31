PY3_LIBRARY()

OWNER(g:voicetech-infra)

PY_SRCS(
    __init__.py
)

PEERDIR(
    contrib/python/aiohttp
    contrib/python/requests
    contrib/python/deepdiff
    contrib/python/mypy-extensions
    contrib/python/tenacity
    contrib/python/PyYAML
)

END()
