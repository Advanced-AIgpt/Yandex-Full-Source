LIBRARY()

OWNER(
    a-square
    akhruslan
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/random_number/nlg
    alice/hollywood/library/scenarios/random_number/proto
    alice/library/analytics/common
    alice/library/json
    alice/library/scled_animations
)

SRCS(
    GLOBAL random_number.cpp
    random_number_scene_rnd.cpp
    random_number_scene_dice.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
    it2
)
