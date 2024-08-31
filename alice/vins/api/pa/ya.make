PY2_PROGRAM()

OWNER(g:alice)

PEERDIR(
    alice/vins/api
    alice/vins/api_helper
    contrib/python/gunicorn
)

PY_SRCS(
    __main__.py
)

END()
