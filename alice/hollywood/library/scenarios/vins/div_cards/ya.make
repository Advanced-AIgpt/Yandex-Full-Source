LIBRARY()

OWNER(g:hollywood)

PEERDIR(
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/protos/data/scenario
    alice/vins/api/vins_api/speechkit/protos
)

SRCS(
    processor.cpp
    show_route.cpp
    show_traffic.cpp
)

END()
