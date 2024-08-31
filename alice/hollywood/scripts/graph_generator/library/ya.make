PY23_LIBRARY()

OWNER(g:hollywood)

PEERDIR(
    contrib/python/Jinja2
    apphost/lib/python_util/conf
)

PY_SRCS(
    graph_generator.py
)

END()

