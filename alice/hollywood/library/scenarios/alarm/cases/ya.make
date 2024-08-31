LIBRARY()

OWNER(
    g:alice-alarm-scenario
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/scenarios/alarm/context
    alice/hollywood/library/scenarios/alarm/nlg
    alice/hollywood/library/scenarios/alarm/proto
    alice/hollywood/library/scenarios/alarm/util
    alice/library/calendar_parser
    alice/library/music
    library/cpp/timezone_conversion
)

SRCS(
    alarm_cases.cpp
    timer_cases.cpp
)

END()
