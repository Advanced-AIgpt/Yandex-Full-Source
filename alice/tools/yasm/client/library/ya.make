PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PY_SRCS(
    __init__.py
    common.py
    types.py
    compare.py
    matching.py
    arguments.py
    charts.py
    aiohttp_client.py
    alerts.py
    dashboard.py
    verify.py
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

RECURSE_FOR_TESTS(
    ut
)
