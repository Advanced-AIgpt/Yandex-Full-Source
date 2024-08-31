PY3_PROGRAM()

OWNER(
    g:paskills
    penguin-diver
)

PY_SRCS(
    __main__.py
    config.py
)

PEERDIR(
    alice/paskills/penguinarium
    contrib/python/PyYAML
    contrib/python/python-json-logger
)

END()
