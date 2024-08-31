PY2_LIBRARY()

OWNER(
    kseniial
    g:alice_quality
)

PEERDIR(
    # 3rd party
    contrib/python/Werkzeug
)

PY_SRCS(
    NAMESPACE verstehen.granet_errors
    __init__.py
    wrong_grammar.py
)

END()
