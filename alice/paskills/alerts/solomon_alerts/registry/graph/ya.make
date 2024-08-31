OWNER(
    g:paskills
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    graphs.py
)

PEERDIR(
    library/python/monitoring/solo/example/example_project/registry/project
    library/python/monitoring/solo/example/example_project/registry/sensor
    library/python/monitoring/solo/objects/solomon/v2
)

END()
