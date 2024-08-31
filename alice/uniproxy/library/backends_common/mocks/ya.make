PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    protostreamserver.py
)

PEERDIR(
    contrib/python/tornado/tornado-4
)

END()
