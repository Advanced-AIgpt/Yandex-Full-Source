PY2_LIBRARY()

OWNER(
    polinakud
)

PEERDIR(
    quality/ab_testing/cofe/projects/alice/heartbeats
    quality/ab_testing/cofe/projects/alice/timespent
    quality/ab_testing/cofe/projects/alice
    alice/analytics/operations/dialog/sessions
)

PY_SRCS(
    NAMESPACE alice.analytics.operations.timespent

    mappings.py
    device_mappings.py
    support_timespent_functions.py
)

END()
