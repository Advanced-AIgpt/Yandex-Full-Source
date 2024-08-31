LIBRARY()

OWNER(
    vkaneva
    g:goods-runtime
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/context
    alice/hollywood/library/frame
    alice/hollywood/library/global_context
    alice/hollywood/library/http_proxy
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/resources
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/goods/nlg
    alice/hollywood/library/scenarios/goods/proto
    alice/library/url_builder
    alice/protos/analytics/goods
)

SRCS(
    GLOBAL goods.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
