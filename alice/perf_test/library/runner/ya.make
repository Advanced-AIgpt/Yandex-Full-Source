PY2_LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/perf_test/library/joker
    library/python/vault_client
)

PY_SRCS(
    __init__.py
    bmv.py
    request.py
    settings.py
)

END()
