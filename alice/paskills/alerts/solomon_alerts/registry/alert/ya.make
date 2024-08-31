OWNER(
    g:paskills
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    dialogovo_alerts.py
    dialogovo_alerts_from_megamind.py
    memento_alerts.py
    paskills_alerts.py
)

PEERDIR(
    library/python/monitoring/solo/example/example_project/registry/project
    library/python/monitoring/solo/example/example_project/registry/channel
    library/python/monitoring/solo/example/example_project/registry/sensor
    library/python/monitoring/solo/example/example_project/registry/shard
    library/python/monitoring/solo/objects/solomon/v2
    library/python/monitoring/solo/helpers
    contrib/python/juggler_sdk
)

END()
