LIBRARY()

OWNER(
    g:alice-alarm-scenario
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/alarm/handles
    alice/hollywood/library/sound
    alice/library/scled_animations
)

SRCS(
    GLOBAL register.cpp
)

END()

RECURSE_FOR_TESTS(it2)
