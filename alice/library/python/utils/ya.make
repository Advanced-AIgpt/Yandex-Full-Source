PY3_LIBRARY()

OWNER(
    g:alice
)

PEERDIR(
    contrib/python/protobuf
)

PY_SRCS(
    __init__.py
    arcadia.py
    network.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
