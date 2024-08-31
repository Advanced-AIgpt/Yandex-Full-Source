UNITTEST_FOR(alice/megamind/configs)

OWNER(
    alkapov
    g:megamind
)

SRCS(
    config_ut_helpers.cpp
    validate_ut.cpp
    validate_combinator_configs_ut.cpp
)

PEERDIR(
    alice/library/network
    alice/megamind/library/classifiers/formulas/protos
    alice/megamind/library/config
    alice/megamind/library/protos
    alice/megamind/library/scenarios/config_registry
    alice/megamind/library/scenarios/defs
    alice/megamind/library/scenarios/helpers
    alice/megamind/library/testing
    alice/megamind/protos/quality_storage
    alice/protos/data/language
    library/cpp/testing/unittest
)

DATA(
    arcadia/alice/megamind/configs/common
    arcadia/alice/megamind/configs/dev
    arcadia/alice/megamind/configs/hamster
    arcadia/alice/megamind/configs/production
    arcadia/alice/megamind/configs/rc
)

END()
