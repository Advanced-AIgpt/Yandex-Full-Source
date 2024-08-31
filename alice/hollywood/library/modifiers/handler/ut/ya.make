UNITTEST_FOR(alice/hollywood/library/modifiers/handler)

OWNER(
    yagafarov
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/modifiers/testing

    alice/library/proto
    alice/library/unittest

    library/cpp/testing/common
    library/cpp/testing/gmock_in_unittest
    library/cpp/testing/unittest

    apphost/lib/service_testing
)

SRCS(
    modifier_handle_ut.cpp
)

END()
