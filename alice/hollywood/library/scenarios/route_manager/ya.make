LIBRARY()

OWNER(
    deemonasd
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/framework/core
    alice/hollywood/library/request
    alice/hollywood/library/scenarios/route_manager/nlg
    alice/hollywood/library/scenarios/route_manager/proto
    alice/library/analytics/common
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/protos/endpoint/capabilities/route_manager
)

SRCS(
    GLOBAL route_manager.cpp
    route_manager_common.cpp
    route_manager_scene_continue.cpp
    route_manager_scene_error.cpp
    route_manager_scene_show.cpp
    route_manager_scene_start.cpp
    route_manager_scene_stop.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
