LIBRARY()

OWNER(g:hollywood)

SRCS(
    metrics.cpp
)

PEERDIR(
    alice/hollywood/library/base_hw_service
    alice/hollywood/library/base_scenario
    alice/hollywood/library/framework/proto
    library/cpp/monlib/metrics
)

GENERATE_ENUM_SERIALIZATION(metrics.h)

END()
