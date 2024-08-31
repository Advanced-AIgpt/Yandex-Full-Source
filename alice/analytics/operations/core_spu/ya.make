PY23_LIBRARY()

OWNER(
    gri201
    g:alice_analytics
)

PEERDIR(
    statbox/nile
    statbox/qb2
    yql/library/python
    alice/analytics/utils
)

PY_SRCS(
    NAMESPACE alice.analytics.operations.core_spu

    activities.py
    utils.py
)

END()
