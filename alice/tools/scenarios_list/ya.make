PY3_PROGRAM()

OWNER(zubchick)

PEERDIR(
    contrib/python/attrs
    contrib/python/click
    contrib/python/requests

    yt/python/yt/wrapper
    yt/yt/python/yt_yson_bindings
)

PY_SRCS(
    MAIN __main__.py
)

END()
