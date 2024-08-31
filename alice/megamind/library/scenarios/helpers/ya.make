LIBRARY()

OWNER(g:megamind)

SRCS(
    scenario_ref.cpp
    scenario_wrapper.cpp
    scenario_api_helper.cpp
    session_view.cpp
)

PEERDIR(
    alice/bass/libs/fetcher
    alice/megamind/library/context
    alice/megamind/library/experiments
    alice/megamind/library/models/directives
    alice/megamind/library/memento
    alice/megamind/library/proactivity/common
    alice/megamind/library/response
    alice/megamind/library/request/event
    alice/megamind/library/scenarios/defs
    alice/megamind/library/scenarios/helpers/interface
    alice/megamind/library/scenarios/helpers/get_request_language
    alice/megamind/library/scenarios/interface
    alice/megamind/library/scenarios/protocol
    alice/megamind/library/scenarios/utils
    alice/megamind/library/serializers
    alice/megamind/library/session/protos
    alice/megamind/library/speechkit
    alice/megamind/library/stack_engine
    alice/megamind/library/util
    alice/megamind/library/worldwide/language
    alice/megamind/protos/common
    alice/megamind/protos/modifiers
    alice/megamind/protos/scenarios
    alice/megamind/protos/speechkit
    alice/library/experiments
    alice/library/metrics
    alice/library/response
    alice/library/version
    kernel/alice/begemot_nlu_features
    library/cpp/http/misc
    library/cpp/protobuf/json
)

END()

RECURSE(
    interface
    get_request_language
)

RECURSE_FOR_TESTS(ut)
