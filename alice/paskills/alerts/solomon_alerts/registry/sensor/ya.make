OWNER(
    g:paskills
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    dialogovo_sensors.py
    megamind_sensors.py
    memento_sensors.py
    paskills_sensors.py
)

PEERDIR(
    library/python/monitoring/solo/objects/solomon/sensor
    library/python/monitoring/solo/example/example_project/registry/project
    library/python/monitoring/solo/example/example_project/registry/cluster
    library/python/monitoring/solo/example/example_project/registry/service
)

END()
