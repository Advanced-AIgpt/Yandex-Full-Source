LIBRARY()

OWNER(
    d-dima
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/blueprints/nlg
    alice/hollywood/library/scenarios/blueprints/proto
)

SRCS(
    GLOBAL blueprints.cpp
    blueprints_fastdata.cpp
    blueprints_scene.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
    it2
)
