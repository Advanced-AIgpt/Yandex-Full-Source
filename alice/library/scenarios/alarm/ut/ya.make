UNITTEST_FOR(alice/library/scenarios/alarm)

OWNER(
    g:alice
    g:alice-alarm-scenario
)

PEERDIR(
    alice/library/unittest
)

SRCS(
    date_ut.cpp
    daytime_ut.cpp
    helpers_ut.cpp
    weekdays_alarm_ut.cpp
)

END()
