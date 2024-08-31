PY3_PROGRAM()

OWNER(g:hollywood)

PEERDIR(
    alice/apphost/graph_generator/scenario
    alice/hollywood/hw/template
    alice/hollywood/library/config
    alice/library/python/server_runner
    alice/library/python/utils
    alice/tests/library/uniclient
    contrib/python/cached-property
    contrib/python/coloredlogs
)

PY_SRCS(
    __main__.py
    service.py
)

END()

RECURSE_FOR_TESTS(build)
