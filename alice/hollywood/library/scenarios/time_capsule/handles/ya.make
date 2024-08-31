LIBRARY()

OWNER(
    g:alice-time-capsule-scenario
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/scenarios/time_capsule/scene

    alice/hollywood/library/base_scenario
    alice/hollywood/library/request
    alice/hollywood/library/response
)

SRCS(
    time_capsule_run.cpp
)

END()
