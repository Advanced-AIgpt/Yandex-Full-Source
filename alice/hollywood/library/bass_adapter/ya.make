LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/bass/libs/request
    alice/hollywood/library/base_scenario
    alice/hollywood/library/http_proxy
    alice/hollywood/library/player_features
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/protos
    alice/library/analytics/common
    alice/library/experiments
    alice/library/json
    alice/library/logger
    alice/library/metrics
    alice/library/network
    alice/megamind/protos/scenarios
    library/cpp/monlib/metrics
    library/cpp/scheme
    apphost/lib/proto_answers
)

SRCS(
    bass_adapter.cpp
    bass_renderer.cpp
    bass_stats.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
