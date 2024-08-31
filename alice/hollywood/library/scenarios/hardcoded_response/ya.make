LIBRARY()

OWNER(
    akhruslan
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/fast_data
    alice/hollywood/library/framework
    alice/hollywood/library/global_context
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/hardcoded_response/nlg
    alice/hollywood/library/scenarios/hardcoded_response/proto
    alice/library/client/protos
    alice/library/logger
    alice/library/proto
    alice/library/scled_animations
    alice/library/util
    alice/megamind/protos/scenarios
    library/cpp/iterator
    library/cpp/regex/pcre
)

SRCS(
    GLOBAL hardcoded_response.cpp
    GLOBAL hardcoded_response_new.cpp
    applicability_wrapper.cpp
    hardcoded_response_fast_data.cpp
)

END()

RECURSE_FOR_TESTS(
    it
    it2
    ut
)
