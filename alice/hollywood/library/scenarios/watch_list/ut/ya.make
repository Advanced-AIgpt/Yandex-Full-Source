UNITTEST_FOR(alice/hollywood/library/scenarios/watch_list)

OWNER(
    g:smarttv
    kolchanovs
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/testing
    alice/hollywood/library/framework
    alice/hollywood/library/framework/unittest
    alice/library/json
    library/cpp/testing/unittest
    library/cpp/testing/gmock_in_unittest
    apphost/lib/service_testing
)

SRCS(
    alice/hollywood/library/scenarios/watch_list/ut/handler_ut.cpp
)

END()
