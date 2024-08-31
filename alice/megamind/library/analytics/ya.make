LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/analytics/interfaces
    alice/library/analytics/scenario
    alice/library/json
    alice/megamind/library/request
    alice/megamind/protos/analytics
    alice/megamind/protos/analytics/modifiers/proactivity
)

SRCS(
    analytics_info.cpp
    megamind_analytics_info.cpp
)

END()

RECURSE_FOR_TESTS(ut)
