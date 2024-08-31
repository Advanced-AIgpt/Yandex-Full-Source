LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/bass/libs/fetcher
    alice/library/logger
    alice/library/metrics
    alice/library/proto
    library/cpp/libgit2_wrapper
    library/cpp/scheme
    library/cpp/testing/gmock_in_unittest
)

SRCS(
    fake_fetcher.cpp
    message_diff.cpp
    mock_request_eventlistener.cpp
    mock_sensors.cpp
    ut_helpers.cpp
)

END()
