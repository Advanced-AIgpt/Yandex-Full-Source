LIBRARY()

OWNER(
    hellodima
    g:smarttv
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/frame
    alice/hollywood/library/global_context
    alice/hollywood/library/http_proxy
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/tv_channels/nlg
    alice/library/analytics/common
    alice/library/logger
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/nlu/libs/request_normalizer
    library/cpp/iterator
    library/cpp/json/writer
)

SRCS(
    common.cpp
    prepare.cpp
    render.cpp
    GLOBAL register.cpp
)

END()

RECURSE_FOR_TESTS(
    it
    it2
)
