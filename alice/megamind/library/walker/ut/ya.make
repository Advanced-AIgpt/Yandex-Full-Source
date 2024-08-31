UNITTEST_FOR(alice/megamind/library/walker)

OWNER(g:megamind)

PEERDIR(
    alice/library/frame
    alice/library/geo
    alice/library/iot
    alice/library/json
    alice/library/logger
    alice/library/metrics
    alice/library/unittest
    alice/library/util
    alice/megamind/library/analytics
    alice/megamind/library/apphost_request
    alice/megamind/library/apphost_request/protos
    alice/megamind/library/classifiers
    alice/megamind/library/context
    alice/megamind/library/experiments
    alice/megamind/library/globalctx
    alice/megamind/library/modifiers
    alice/megamind/library/requestctx
    alice/megamind/library/response
    alice/megamind/library/scenarios/features/protos
    alice/megamind/library/scenarios/helpers
    alice/megamind/library/scenarios/helpers/interface
    alice/megamind/library/scenarios/interface
    alice/megamind/library/scenarios/protocol
    alice/megamind/library/scenarios/registry
    alice/megamind/library/scenarios/registry/interface
    alice/megamind/library/session
    alice/megamind/library/testing
    alice/megamind/library/util
    alice/megamind/nlg
    alice/megamind/protos/common
    alice/megamind/protos/quality_storage
    alice/megamind/protos/scenarios
    alice/megamind/protos/speechkit
    alice/nlg/library/nlg_renderer
    apphost/lib/service_testing
    kernel/factor_storage
    library/cpp/json
    library/cpp/langs
    library/cpp/testing/gmock_in_unittest
)

SIZE(MEDIUM)

SRCS(
    event_count_ut.cpp
    response_visitor_ut.cpp
    request_frame_to_scenario_matcher_ut.cpp
    talkien_ut.cpp
    walker_ut.cpp
)

DATA(
    sbr://3069941764 # geodata6.bin
)

REQUIREMENTS(ram:10)

END()
