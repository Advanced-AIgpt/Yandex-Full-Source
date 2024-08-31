PY23_LIBRARY()

OWNER(g:jupyter-cloud)

PEERDIR(
    library/python/nirvana
    library/python/vault_client
)

PY_SRCS(
    __init__.py
    operation.py
    yav.py
    compat.py
)

IF(PYTHON2)
PEERDIR(
    contrib/python/pathlib2
    contrib/deprecated/python/backports.functools-lru-cache
    contrib/python/cached-property
)
ENDIF()

END()
