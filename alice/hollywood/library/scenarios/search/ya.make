LIBRARY()

OWNER(
    akhruslan
    tolyandex
    g:alice
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/framework
    alice/hollywood/library/framework/helpers/nlu_features
    alice/hollywood/library/global_context
    alice/hollywood/library/http_proxy
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/search/context
    alice/hollywood/library/scenarios/search/nlg
    alice/hollywood/library/scenarios/search/scenarios
    alice/hollywood/library/scenarios/search/scenes
    alice/library/experiments
    alice/library/json
    alice/library/restriction_level
    alice/library/scenarios/data_sources
    alice/megamind/protos/scenarios
)

SRCS(
    GLOBAL handles.cpp
    GLOBAL search_dispatcher.cpp
)

END()

RECURSE_FOR_TESTS(
    it
    it2
)
