LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/client
    alice/megamind/protos/scenarios
    library/cpp/monlib/metrics
    library/cpp/monlib/service/pages
    library/cpp/unistat
    library/cpp/unistat/idl
)

SRCS(
    aggregate_labels_builder.cpp
    gauge.cpp
    histogram.cpp
    names.cpp
    sensors.cpp
    service.cpp
    unistat.cpp
    util.cpp
    sensors_queue.cpp
)

GENERATE_ENUM_SERIALIZATION(names.h)

END()

RECURSE(
    sensors_dumper
)

RECURSE_FOR_TESTS(ut)
