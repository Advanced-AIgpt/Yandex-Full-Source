PROGRAM(metrics_aggregator)

OWNER(
    akhruslan
)

PEERDIR(
    alice/library/json
    alice/library/metrics
    alice/library/metrics/sensors_dumper
    alice/tools/metrics_aggregator/library
    apphost/api/service/cpp
    library/cpp/getopt/small
    library/cpp/monlib/metrics
    library/cpp/neh
)

SRCS(
    main.cpp
)

END()
