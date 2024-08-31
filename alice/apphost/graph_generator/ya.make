OWNER(g:hollywood)

PY3_PROGRAM()

PEERDIR(
    alice/apphost/graph_generator/combinator
    alice/apphost/graph_generator/rpc_handler
    alice/apphost/graph_generator/scenario
    alice/library/python/utils
    alice/megamind/library/config/scenario_protos
)

PY_SRCS(
    __main__.py
)

END()
