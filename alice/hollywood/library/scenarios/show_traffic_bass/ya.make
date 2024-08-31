LIBRARY()

OWNER(
    yagafarov
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/global_context
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/show_traffic_bass/nlg
    alice/hollywood/library/scenarios/show_traffic_bass/proto
    alice/library/json
    alice/megamind/protos/scenarios
)

SRCS(
    prepare_with_bass.cpp
    renderer.cpp
    render_with_bass.cpp
    GLOBAL show_traffic.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
    ut
)
