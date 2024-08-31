LIBRARY()

OWNER(
    {{username}}
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/{{scenario_name}}/nlg
    alice/hollywood/library/scenarios/{{scenario_name}}/proto
)

SRCS(
    GLOBAL {{scenario_name}}.cpp
    {{scenario_name}}_scene.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
    it2
)
