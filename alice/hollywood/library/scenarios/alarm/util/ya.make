LIBRARY()

OWNER(
    g:alice-alarm-scenario
    g:hollywood
    g:alice
)

PEERDIR(
    alice/library/apphost_request
    alice/library/music
    alice/library/scenarios/alarm
    alice/hollywood/library/scenarios/alarm/context
)

SRCS(
    util.cpp
)

END()
