LIBRARY()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/vins/div_cards
    alice/hollywood/library/scenarios/vins/proto
    alice/hollywood/library/vins
    alice/vins/api/vins_api/speechkit/protos
    library/cpp/cgiparam
)

SRCS(
    GLOBAL vins_dispatcher.cpp
    vins_scene.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
