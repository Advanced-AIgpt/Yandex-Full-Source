PY3_PROGRAM()

OWNER(
    alkapov
)

PY_SRCS(
    __main__.py
    rules.py
)

PEERDIR(
    contrib/python/click

    yt/python/client
)

END()
