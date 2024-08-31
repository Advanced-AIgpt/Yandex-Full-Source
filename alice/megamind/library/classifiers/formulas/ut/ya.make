UNITTEST_FOR(alice/megamind/library/classifiers/formulas)

OWNER(
    olegator
    g:megamind
)

PEERDIR(
    alice/library/logger
    alice/library/unittest
    alice/megamind/library/classifiers/formulas/protos
    alice/megamind/library/config
    alice/megamind/library/scenarios/defs
    alice/megamind/library/testing
    alice/protos/data/language
    library/cpp/langs
    library/cpp/protobuf/util
)

SRCS(
    formulas_description_ut.cpp
)

DATA(
    arcadia/alice/megamind/configs/common
    arcadia/alice/megamind/configs/production
    arcadia/alice/megamind/library/classifiers/formulas/ut
)

END()
