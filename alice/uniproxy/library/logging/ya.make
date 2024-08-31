PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    log.py
)

PEERDIR(
    contrib/python/tornado/tornado-4
)

END()

RECURSE_FOR_TESTS(ut)
