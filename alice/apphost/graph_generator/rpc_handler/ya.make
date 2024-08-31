OWNER(g:hollywood)

PY3_LIBRARY()

PEERDIR(
    alice/apphost/graph_generator/template
    alice/megamind/library/config/scenario_protos
    alice/tests/library/uniclient
    contrib/python/cached-property
)

PY_SRCS(
    __init__.py
    rpc_handler.py
)

END()
