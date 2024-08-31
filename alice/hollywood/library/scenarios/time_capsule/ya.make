LIBRARY()

OWNER(
    g:alice-time-capsule-scenario
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/registry
    alice/hollywood/library/scenarios/time_capsule/context
    alice/hollywood/library/scenarios/time_capsule/handles
    alice/hollywood/library/scenarios/time_capsule/nlg
    alice/hollywood/library/scenarios/time_capsule/proto
    alice/hollywood/library/scenarios/time_capsule/scene
    alice/hollywood/library/scenarios/time_capsule/util
)

SRCS(
    GLOBAL register.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
