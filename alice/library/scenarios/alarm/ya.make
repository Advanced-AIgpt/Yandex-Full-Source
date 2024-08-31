LIBRARY()

OWNER(
    g:alice
    g:alice-alarm-scenario
)

PEERDIR(
    alice/library/calendar_parser

    library/cpp/timezone_conversion
)

SRCS(
    date_time.cpp
    helpers.cpp
    weekday.cpp
    weekdays.cpp
    weekdays_alarm.cpp
)

GENERATE_ENUM_SERIALIZATION(date_time.h)

END()

RECURSE_FOR_TESTS(
    ut
)
