LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/bass/libs/scheduler
    alice/megamind/library/classifiers/formulas
    alice/megamind/library/config
    alice/megamind/library/factor_storage
    alice/megamind/library/globalctx
    alice/megamind/library/scenarios/registry
    alice/megamind/nlg

    alice/library/metrics
    alice/library/util
    alice/nlg/library/nlg_renderer

    catboost/libs/model

    kernel/formula_storage

    logbroker/unified_agent/client/cpp
    logbroker/unified_agent/client/cpp/logger
)

SRCS(
    globalctx.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
