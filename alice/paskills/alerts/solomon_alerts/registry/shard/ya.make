OWNER(
    g:paskills
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    shards.py
)

PEERDIR(
    library/python/monitoring/solo/objects/solomon/v2
    library/python/monitoring/solo/example/example_project/registry/project
    library/python/monitoring/solo/example/example_project/registry/cluster
    library/python/monitoring/solo/example/example_project/registry/service
)

END()
