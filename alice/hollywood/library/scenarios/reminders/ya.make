LIBRARY()

OWNER(
    petrk
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/frame
    alice/hollywood/library/framework
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/reminders/nlg
    alice/hollywood/library/scenarios/reminders/proto
    alice/hollywood/library/sound
    alice/hollywood/library/vins
    alice/library/analytics/common
    alice/library/device_state
    alice/library/frame
    alice/library/proto
    alice/library/scenarios/reminders
    alice/megamind/protos/scenarios
    alice/megamind/protos/speechkit
    alice/protos/data/scenario/reminders
    alice/vins/api/vins_api/speechkit/protos
    library/cpp/timezone_conversion
)

SRCS(
    GLOBAL registry.cpp
    GLOBAL reminders.cpp
    entry_point.cpp
    result.cpp

    scene/old_flow.cpp
    scene/vins.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
