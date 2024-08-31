LIBRARY()

OWNER(
    g:megamind
    nkodosov
)

PEERDIR(
    alice/library/skill_discovery
    alice/megamind/library/config
    alice/megamind/library/sources
    alice/megamind/protos/scenarios
    saas/api/search_client
)

SRCS(
    saas.cpp
)

END()

RECURSE_FOR_TESTS(ut)
