PY3_LIBRARY()

OWNER(
    sparkle
    g:alice
)

PEERDIR(
    contrib/python/requests
    contrib/python/retry
)

PY_SRCS(
    __init__.py
    evo_parser_data.proto
    evo_parser_lib.py
)

END()
