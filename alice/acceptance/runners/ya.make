PY3_PROGRAM(run)

OWNER(g:alice)

PEERDIR(
    contrib/python/AttrDict
    contrib/python/click
    contrib/python/deepmerge
    contrib/python/PyYAML
    contrib/python/scipy
    library/python/resource
    nirvana/valhalla/src
    yt/python/client
)

PY_SRCS(
    MAIN acceptance_vh_run.py
    check_analytics_info/run.py
    common_operations.py
    config.py
)

RESOURCE(
    config.yaml /config.yaml
)

END()
