LIBRARY()

OWNER(g:megamind)

SRCS(
    blackbox.cpp
    data_sources.cpp
    protocol_scenario.cpp
    response_builder.cpp
    scenario.cpp
    scenario_env.cpp
    quasar_devices_info.cpp
)

GENERATE_ENUM_SERIALIZATION(scenario.h)

PEERDIR(
    alice/bass/libs/fetcher
    alice/library/analytics/common
    alice/library/analytics/interfaces
    alice/library/blackbox/proto
    alice/library/scenarios/data_sources
    alice/megamind/library/apphost_request
    alice/megamind/library/config
    alice/megamind/library/context
    alice/megamind/library/models/directives
    alice/megamind/library/models/interfaces
    alice/megamind/library/raw_responses
    alice/megamind/library/scenarios/features
    alice/megamind/library/scenarios/utils
    alice/megamind/library/session
    alice/megamind/library/sources
    alice/megamind/library/util
    alice/megamind/protos/blackbox
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/protos/data
    library/cpp/langs
    kernel/factor_storage
)

END()

RECURSE_FOR_TESTS(ut)
