LIBRARY()

OWNER(
    alexanderplat
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/music_what_is_playing/nlg
    alice/hollywood/library/scenarios/music_what_is_playing/proto

    alice/hollywood/library/vins
    alice/library/analytics/common
    alice/vins/api/vins_api/speechkit/protos
)

SRCS(
    GLOBAL music_what_is_playing.cpp
    vins_scene.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
    it2
)
