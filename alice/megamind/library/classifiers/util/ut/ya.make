UNITTEST_FOR(alice/megamind/library/classifiers/util)

OWNER(
    makatunkin
    g:megamind
)

PEERDIR(
    alice/protos/data/language
    kernel/formula_storage
    library/cpp/testing/unittest
)

SRCS(
    experiments_ut.cpp
    thresholds_ut.cpp
)

END()
