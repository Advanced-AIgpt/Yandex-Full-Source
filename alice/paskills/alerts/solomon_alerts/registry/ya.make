OWNER(
    g:paskills
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
)

PEERDIR(
    alice/paskills/alerts/solomon_alerts/registry/alert
    alice/paskills/alerts/solomon_alerts/registry/graph
    alice/paskills/alerts/solomon_alerts/registry/channel
    alice/paskills/alerts/solomon_alerts/registry/cluster
    alice/paskills/alerts/solomon_alerts/registry/dashboard
    alice/paskills/alerts/solomon_alerts/registry/project
    alice/paskills/alerts/solomon_alerts/registry/sensor
    alice/paskills/alerts/solomon_alerts/registry/service
    alice/paskills/alerts/solomon_alerts/registry/shard
)

END()

RECURSE(
    alert
    channel
    cluster
    dashboard
    graph
    project
    sensor
    service
    shard
)
