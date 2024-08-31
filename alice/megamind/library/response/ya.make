LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/analytics/scenario
    alice/library/experiments
    alice/library/json
    alice/library/response
    alice/megamind/api/response
    alice/megamind/library/analytics
    alice/megamind/library/experiments
    alice/megamind/library/models/directives
    alice/megamind/library/models/interfaces
    alice/megamind/library/request/event
    alice/megamind/library/response/proto
    alice/megamind/library/scenarios/config_registry
    alice/megamind/library/scenarios/features
    alice/megamind/library/scenarios/interface
    alice/megamind/library/serializers
    alice/megamind/library/session
    alice/megamind/library/speechkit
    alice/megamind/library/util
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/megamind/protos/speechkit
    library/cpp/http/misc
    library/cpp/json
    library/cpp/protobuf/json
)

SRCS(
    builder.cpp
    combinator_response.cpp
    response.cpp
    utils.cpp
)

END()

RECURSE_FOR_TESTS(ut)
