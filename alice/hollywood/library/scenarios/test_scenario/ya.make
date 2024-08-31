LIBRARY()

OWNER(
    d-dima
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/test_scenario/nlg
    alice/hollywood/library/scenarios/test_scenario/proto

    alice/library/json
)

SRCS(
    GLOBAL test_scenario.cpp
    test_scenario_scene.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
