PY3_PROGRAM()

OWNER(
    samoylovboris
    g:alice_quality
)

PEERDIR(
    contrib/python/click
    library/python/json
    yt/python/client
)

PY_SRCS(
    MAIN __main__.py
    utils.py
)

END()
