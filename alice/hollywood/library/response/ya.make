LIBRARY()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/context
    alice/hollywood/library/framework/core
    alice/hollywood/library/global_context
    alice/hollywood/library/nlg
    alice/library/analytics/interfaces
    alice/library/analytics/scenario
    alice/library/factors
    alice/library/json
    alice/library/response
    alice/library/response_similarity
    alice/library/util
    alice/library/version
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/megamind/protos/scenarios/features
    alice/protos/api/renderer
    alice/protos/data/scenario/centaur
    library/cpp/json
    library/cpp/langs
    library/cpp/scheme
)

SRCS(
    music_features.cpp
    push.cpp
    response_builder.cpp
    schedule_action.cpp
)

END()
