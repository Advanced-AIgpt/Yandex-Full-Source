PY3_LIBRARY()

PY_SRCS(
    data.py
    dispatcher.py
    manifest.py
    model.py
    parser.py
)

PEERDIR(
    contrib/python/attrs

    yt/python/client
)

END()
