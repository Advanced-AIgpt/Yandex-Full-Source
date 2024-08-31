PY3_LIBRARY()

OWNER(g:alice)

PEERDIR(
    contrib/python/attrs
    contrib/python/click
    contrib/python/pytz
    library/python/resource
    nirvana/valhalla/src
)

PY_SRCS(
    common.py
    deep_graph.py
    lazy_helpers.py
    op.py
)

END()
