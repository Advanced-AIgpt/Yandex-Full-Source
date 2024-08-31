PY2_LIBRARY()

OWNER(
    artemkorenev
    g:alice_quality
)

PEERDIR(
    # 3rd party
    nirvana/valhalla/src
)

PY_SRCS(
    NAMESPACE verstehen.markup
    __init__.py
    markup_graph_launcher.py
)

END()
