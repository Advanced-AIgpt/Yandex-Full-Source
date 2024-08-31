LIBRARY()

OWNER(
    g:alice
)

SRCS(
    factors.cpp
    response.cpp
)

PEERDIR(
    alice/library/logger
    alice/megamind/protos/scenarios
    alice/protos/websearch
    kernel/blender/factor_storage
    library/cpp/scheme
    search/begemot/rules/query_factors/proto
)

END()

RECURSE_FOR_TESTS(
    ut
)
