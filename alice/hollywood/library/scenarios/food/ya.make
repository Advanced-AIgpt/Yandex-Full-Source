LIBRARY()

OWNER(
    samoylovboris
    the0
    g:alice_quality
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/food/backend
    alice/hollywood/library/scenarios/food/backend/proto
    alice/hollywood/library/scenarios/food/menu_matcher
    alice/hollywood/library/scenarios/food/nlg
    alice/hollywood/library/scenarios/food/proto
    alice/library/json
    alice/library/parsed_user_phrase
    alice/library/proto
    alice/library/version
    alice/megamind/library/util
    alice/megamind/protos/scenarios
    alice/nlu/granet/lib/utils
    alice/nlu/libs/request_normalizer
    library/cpp/iterator
)

SRCS(
    dialog_config.cpp
    food_commit.cpp
    food_run.cpp
    handy_response_builder.cpp
    GLOBAL register.cpp
)

END()

RECURSE(
    backend
    menu_matcher
    nlg
    proto
)

RECURSE_FOR_TESTS(
    ut
    it2
)
