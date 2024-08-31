LIBRARY()

OWNER(
    olegator
    g:megamind
)

PEERDIR(
    alice/library/video_common
    alice/library/logger
    alice/megamind/library/classifiers/defs
    alice/megamind/library/classifiers/formulas
    alice/megamind/library/classifiers/formulas/protos
    alice/megamind/library/classifiers/util
    alice/megamind/library/context
    alice/megamind/library/experiments
    alice/megamind/library/scenarios/config_registry
    alice/megamind/library/scenarios/defs
    kernel/factor_storage
    library/cpp/langs
)

SRCS(
    matrixnet.cpp
    priority.cpp
)

END()

RECURSE_FOR_TESTS(ut)
