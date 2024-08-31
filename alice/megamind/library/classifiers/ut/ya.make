UNITTEST_FOR(alice/megamind/library/classifiers)

OWNER(
    olegator
    g:megamind
)

PEERDIR(
    alice/library/experiments
    alice/library/frame
    alice/library/json
    alice/library/music
    alice/library/unittest
    alice/library/video_common
    alice/megamind/library/classifiers/formulas
    alice/megamind/library/classifiers/util
    alice/megamind/library/common
    alice/megamind/library/config
    alice/megamind/library/context
    alice/megamind/library/experiments
    alice/megamind/library/request
    alice/megamind/library/request/event
    alice/megamind/library/response
    alice/megamind/library/scenarios/config_registry
    alice/megamind/library/scenarios/defs
    alice/megamind/library/scenarios/helpers/interface
    alice/megamind/library/scenarios/protocol
    alice/megamind/library/scenarios/registry
    alice/megamind/library/session
    alice/megamind/library/testing
    alice/megamind/library/vins
    alice/megamind/protos/common

    alice/protos/data/language

    kernel/factor_storage
    kernel/formula_storage
    kernel/geodb
    library/cpp/testing/gmock_in_unittest
)

DATA(
    sbr://2493209730=formulas
    sbr://2077646575 # partial_preclf_model.cbm
    arcadia/alice/megamind/configs/common
    arcadia/alice/megamind/configs/production
)

SIZE(MEDIUM)

SRCS(
    pre_ut.cpp
    post_ut.cpp
)

REQUIREMENTS(ram:9)

END()
