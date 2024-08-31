LIBRARY()

OWNER(
    akhruslan
)

PEERDIR(
    alice/bass/libs/scheduler
    alice/library/metrics
    alice/library/metrics/sensors_dumper
    alice/tools/metrics_aggregator/library
    library/cpp/monlib/metrics
    library/cpp/neh
)

SRCS(
    metrics_proxy.cpp
)

END()
