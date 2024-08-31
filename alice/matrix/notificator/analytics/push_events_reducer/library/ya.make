LIBRARY()

OWNER(
    g:matrix
)

PEERDIR(
    alice/matrix/notificator/analytics/common
    
    alice/matrix/analytics/enum_value_ordering

    alice/matrix/library/logging/events
    
    library/cpp/eventlog
)

SRCS(
    aggregated_events_info.cpp
    enum_value_ordering.cpp
    mapreduce_by_push_id.cpp
    reducer.cpp
    table_helper.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
