LIBRARY()

OWNER(
    g:hollywood
    g:maps-mobile-alice
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/global_context
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/resources
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/maps_download_offline/nlg
    alice/library/geo
    alice/library/proto
    alice/library/url_builder
    library/cpp/string_utils/quote
)

SRCS(
    GLOBAL maps_download_offline.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
