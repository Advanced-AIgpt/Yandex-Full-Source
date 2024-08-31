LIBRARY()

OWNER(
    d-dima
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/search/proto
    alice/hollywood/library/scenarios/search/scenes/processors
    alice/library/search_result_parser
    alice/megamind/protos/analytics/scenarios/search
)

SRCS(
    old_flow.cpp
    screendevice_scene.cpp
)

END()
