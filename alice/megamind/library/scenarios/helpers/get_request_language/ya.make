LIBRARY()

OWNER(g:megamind)

SRCS(
    get_scenario_request_language.cpp
)

PEERDIR(
    alice/megamind/library/config/scenario_protos
    alice/megamind/library/context
    alice/megamind/library/experiments
    alice/megamind/library/worldwide/language
)

END()

RECURSE_FOR_TESTS(
    ut
)
