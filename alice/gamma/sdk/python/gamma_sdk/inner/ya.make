OWNER(
    g-kostin
    g:alice
)

PY3_LIBRARY()

PEERDIR(
    contrib/python/attrs
    contrib/python/ply
)

PY_SRCS(
    NAMESPACE gamma_sdk.inner

    __init__.py
    lexer.py
    matcher.py
    parser.py
)

END()
