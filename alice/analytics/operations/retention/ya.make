PY2_LIBRARY()

OWNER(
    gri201
    g:alice_analytics
)

PEERDIR(
    statbox/nile
    statbox/qb2
    yql/library/python
)

PY_SRCS(
    NAMESPACE alice.analytics.operations.retention

    constants.py
    stations.py
    tvandroid.py
    features/alice_features.py
    features/subscriptions.py
    features/iot_features.py
)

END()
