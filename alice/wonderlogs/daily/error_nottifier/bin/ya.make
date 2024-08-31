PY3_PROGRAM(error_nottifier)

OWNER(g:wonderlogs)

PEERDIR(
    alice/wonderlogs/daily/error_nottifier/lib

    contrib/python/click

    yt/python/client
)

PY_SRCS(__main__.py)

END()
