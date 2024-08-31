UNITTEST_FOR(alice/bass/libs/fetcher)

OWNER(g:bass)

FORK_SUBTESTS()

SRCS(
    multirequest_ut.cpp
    neh_detail_ut.cpp
    neh_ut.cpp
    request_ut.cpp
)

PEERDIR(
    alice/bass/libs/ut_helpers
    library/cpp/http/misc
    library/cpp/neh
    library/cpp/testing/gmock_in_unittest
    library/cpp/threading/future
)

REQUIREMENTS(ram:9)

END()
