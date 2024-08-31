PY2_LIBRARY()

OWNER(g:alice)

PEERDIR(
    contrib/python/falcon
    contrib/python/gunicorn
    contrib/python/raven
    alice/rtlog/client/python/lib
    alice/vins/core
)

PY_SRCS(
    __init__.py
    error.py
    gunicorn_app.py
    resources.py
    standard_settings.py
)

END()
