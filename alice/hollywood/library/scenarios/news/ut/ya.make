UNITTEST_FOR(alice/hollywood/library/scenarios/news)

OWNER(
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/testing
    alice/hollywood/library/framework/unittest
    alice/library/json
    alice/megamind/library/util
    library/cpp/testing/gmock_in_unittest
    apphost/lib/service_testing
)

SRCS(
    alice/hollywood/library/scenarios/news/prepare_handle_ut.cpp
    alice/hollywood/library/scenarios/news/date_helper_ut.cpp
)

END()
