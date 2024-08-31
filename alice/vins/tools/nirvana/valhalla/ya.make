PY2_PROGRAM(vh_run)

OWNER(mkamalova)

PEERDIR(
    contrib/python/attrs
    contrib/python/click
    nirvana/valhalla/src
)

PY_SRCS(
    __main__.py
    global_options.py
    op.py
    vh_arcadia_build.py
    vins_graph.py
    yql.py
)

END()
