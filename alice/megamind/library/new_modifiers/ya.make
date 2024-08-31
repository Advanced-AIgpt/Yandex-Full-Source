LIBRARY()

OWNER(
    yagafarov
    g:megamind
)

PEERDIR(
    alice/megamind/library/analytics
    alice/megamind/library/context
    alice/megamind/library/experiments
    alice/megamind/library/request
    alice/megamind/library/response
    alice/megamind/library/scenarios/helpers/get_request_language
    alice/megamind/library/util

    alice/megamind/library/config/scenario_protos
    alice/megamind/protos/scenarios
    alice/megamind/protos/modifiers

    alice/library/experiments
    alice/library/network
    alice/library/unittest
    alice/library/util
)

SRCS(
    modifier_request_factory.cpp
    utils.cpp
)

END()

RECURSE_FOR_TESTS(ut)
