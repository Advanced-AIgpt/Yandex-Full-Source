PY23_LIBRARY()

OWNER(zubchick)

PEERDIR(
    contrib/python/attrs
    contrib/python/gevent
    contrib/python/PyYAML
)

PY_SRCS(
    __init__.py
    events.py
    state.py
)

END()
