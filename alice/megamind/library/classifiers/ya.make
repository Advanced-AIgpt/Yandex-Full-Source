LIBRARY()

OWNER(
    olegator
    g:megamind
)

PEERDIR(
    alice/library/client
    alice/library/experiments
    alice/library/frame
    alice/library/logger
    alice/library/metrics
    alice/library/music
    alice/library/video_common
    alice/megamind/library/classifiers/defs
    alice/megamind/library/classifiers/features
    alice/megamind/library/classifiers/formulas
    alice/megamind/library/classifiers/intents_by_priority
    alice/megamind/library/classifiers/rankers
    alice/megamind/library/classifiers/util
    alice/megamind/library/context
    alice/megamind/library/experiments
    alice/megamind/library/request
    alice/megamind/library/response
    alice/megamind/library/scenarios/defs
    alice/megamind/library/scenarios/helpers/get_request_language
    alice/megamind/library/scenarios/helpers/interface
    alice/megamind/library/util
    alice/megamind/library/vins
    alice/megamind/library/worldwide/language
    alice/megamind/protos/partials_pre
    alice/megamind/protos/quality_storage
    catboost/libs/model
    kernel/factor_storage
    library/cpp/iterator
    library/cpp/langs
)

SRCS(
    post.cpp
    pre.cpp
)

END()

RECURSE(
    defs
    features
    formulas
    intents_by_priority
    rankers
    util
)

RECURSE_FOR_TESTS(ut)
