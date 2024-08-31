UNITTEST_FOR(alice/hollywood/library/scenarios/hardcoded_response)

OWNER(g:hollywood)

SRCS(
    alice/hollywood/library/scenarios/hardcoded_response/applicability_wrapper_ut.cpp
    alice/hollywood/library/scenarios/hardcoded_response/hardcoded_response_fast_data_ut.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/hardcoded_response
    alice/hollywood/library/testing
    library/cpp/testing/common
    library/cpp/testing/gmock_in_unittest
    library/cpp/testing/unittest
)

END()
