LIBRARY()

OWNER(
    g:megamind
)

PEERDIR(
    alice/library/client
    alice/library/logger
    alice/megamind/library/common
    alice/megamind/library/context
    alice/megamind/library/experiments
    alice/megamind/protos/quality_storage
    alice/megamind/library/classifiers/formulas
    alice/megamind/library/experiments
    alice/megamind/library/scenarios/defs
    alice/megamind/library/util
    library/cpp/langs
)

SRCS(
    experiments.cpp
    force_response.cpp
    modes.cpp
    scenario_info.cpp
    scenario_specific.cpp
    table.cpp
    thresholds.cpp
)

GENERATE_ENUM_SERIALIZATION(table.h)

END()

RECURSE_FOR_TESTS(ut)
