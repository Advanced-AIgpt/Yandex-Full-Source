LIBRARY()

OWNER(
    g:alice-alarm-scenario
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/music
    alice/hollywood/library/scenarios/alarm/cases
)

SRCS(
    alarm_prepare.cpp
    alarm_prepare_music_catalog.cpp
    alarm_run.cpp
)

END()
