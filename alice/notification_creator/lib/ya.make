PY3_LIBRARY()

OWNER(
    g:mediaalice
)

PY_SRCS(
    app.py
    notification.py
)

PEERDIR(
    yt/python/client
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/uniproxy/library/protos
    contrib/python/Flask
    contrib/python/gevent
    contrib/python/gunicorn
    contrib/python/requests
)

END()
