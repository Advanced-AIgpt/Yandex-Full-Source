GTEST()

OWNER(
    petrk
)

SRCS(
    api_ut.cpp
    datetime_ut.cpp
    memento_ut.cpp
    schedule_ut.cpp
    ut.cpp
)

PEERDIR(
    alice/library/scenarios/reminders
)

END()
