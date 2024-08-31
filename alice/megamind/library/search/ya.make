LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/bass/libs/fetcher
    alice/library/analytics/common
    alice/library/experiments
    alice/library/logger
    alice/library/metrics
    alice/library/network
    alice/library/proto
    alice/library/util
    alice/library/websearch
    alice/library/websearch/response
    alice/megamind/library/experiments
    alice/megamind/library/search/protos
    alice/megamind/library/sources
    alice/megamind/library/speechkit
    alice/megamind/library/util
    kernel/blender/factor_storage
    library/cpp/geobase
    library/cpp/scheme
    library/cpp/string_utils/base64
    search/begemot/rules/query_factors/proto
)

SRCS(
    request.cpp
    search.cpp
)

END()

RECURSE_FOR_TESTS(ut)
