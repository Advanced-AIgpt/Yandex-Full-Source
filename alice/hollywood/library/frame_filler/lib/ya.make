LIBRARY()

OWNER(
    g:megamind
)

PEERDIR(
    alice/hollywood/library/request
    alice/library/logger
    alice/library/proto
    alice/library/response
    alice/megamind/library/scenarios/defs
    alice/megamind/library/util
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/hollywood/library/frame_filler/proto
    apphost/api/service/cpp
)

SRCS(
    frame_filler_handlers.cpp
    frame_filler_scenario_handlers.cpp
    frame_filler_utils.cpp
)

END()

RECURSE_FOR_TESTS(ut)
