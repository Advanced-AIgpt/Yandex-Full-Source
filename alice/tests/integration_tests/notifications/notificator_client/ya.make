PY3_LIBRARY()

OWNER(tolyandex)

PY_SRCS(
    __init__.py
)

PEERDIR(
    contrib/python/requests
    alice/uniproxy/library/protos
)

END()
