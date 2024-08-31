UNITTEST_FOR(alice/megamind/library/classifiers/rankers)

OWNER(
    olegator
    g:megamind
)

PEERDIR(
    alice/megamind/library/classifiers/formulas
    alice/megamind/library/factor_storage
    alice/protos/data/language
    kernel/formula_storage
)

# megamind_formulas
DATA(sbr://2327014576)

SRCS(
    matrixnet_ut.cpp
    priority_ut.cpp
)

REQUIREMENTS(ram:12)

END()
