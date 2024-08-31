LIBRARY()

OWNER(
    alexanderplat
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/get_time/nlg
    alice/hollywood/library/scenarios/get_time/proto
    alice/hollywood/library/vins
)

SRCS(
    GLOBAL get_time.cpp
    vins_generic_scene.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
    it2
)
