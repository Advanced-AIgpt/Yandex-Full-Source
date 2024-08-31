LIBRARY()

OWNER(
    g:alice-time-capsule-scenario
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/scenarios/time_capsule/context
    alice/hollywood/library/scenarios/time_capsule/nlg
    alice/hollywood/library/scenarios/time_capsule/proto
    alice/hollywood/library/scenarios/time_capsule/util

    alice/hollywood/library/response
    alice/hollywood/library/scenarios/time_capsule/proto

    library/cpp/timezone_conversion
)

SRCS(
    scene.cpp
    scene_creators.cpp
)

END()
