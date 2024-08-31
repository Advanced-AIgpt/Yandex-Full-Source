UNITTEST_FOR(alice/hollywood/library/scenarios/get_date)

OWNER(
    d-dima
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/framework
    alice/hollywood/library/framework/unittest
    alice/hollywood/library/testing
    alice/hollywood/library/scenarios/get_date/proto
    alice/library/json
    apphost/lib/service_testing
    library/cpp/testing/gmock_in_unittest
)


SRCS(
    slot_utils_ut.cpp
    calendar_utils_ut.cpp
    get_date_dispatch_ut.cpp
    get_date_render_ut.cpp
)

END()
