PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    gcmon.py
    profiling.py
)

PEERDIR(
    contrib/python/tornado/tornado-4
    contrib/python/yappi

    alice/uniproxy/library/logging
    alice/uniproxy/library/settings
)

END()
