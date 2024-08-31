PY3_LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/joker/library/python
    contrib/python/pytz
    contrib/python/retry
    library/python/vault_client
)

PY_SRCS(
    __init__.py
    conftest.py
    megamind.py
    request.py
    response.py
    session.py
    settings.py
    ya_tool.py
)

END()
