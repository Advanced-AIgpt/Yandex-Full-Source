PY3_PROGRAM(event_poster)

OWNER(sparkle)

PEERDIR(
    contrib/python/requests
    library/python/reactor/client
)

PY_SRCS(
    __main__.py
)

END()
