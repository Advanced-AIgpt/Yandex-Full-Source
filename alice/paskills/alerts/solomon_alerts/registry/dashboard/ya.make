OWNER(
    g:paskills
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    dashboards.py
)

PEERDIR(
    library/python/monitoring/solo/example/example_project/registry/project
    library/python/monitoring/solo/example/example_project/registry/graph
    library/python/monitoring/solo/objects/solomon/v2
    library/python/monitoring/solo/helpers
)

END()
