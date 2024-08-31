LIBRARY()

OWNER(
    jan-fazli
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/http_proxy
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/subscriptions_manager/nlg
    alice/hollywood/library/scenarios/subscriptions_manager/proto
    alice/library/app_navigation
    alice/library/billing
    alice/library/json
    alice/library/logger
    alice/library/url_builder
    apphost/lib/proto_answers
    library/cpp/string_utils/quote
)

SRCS(
    GLOBAL subscriptions_manager.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
