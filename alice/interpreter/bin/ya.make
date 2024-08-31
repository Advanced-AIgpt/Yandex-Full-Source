PY3_PROGRAM(interpreter_server)

OWNER(zubchick)

PEERDIR(
    alice/interpreter/lib

    contrib/python/click
    contrib/python/gunicorn
    contrib/python/gevent
)

PY_SRCS(
    MAIN main.py
)

END()
