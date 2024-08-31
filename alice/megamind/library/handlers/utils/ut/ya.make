UNITTEST_FOR(alice/megamind/library/handlers/utils)

OWNER(g:megamind)

PEERDIR(
    alice/megamind/library/analytics

    alice/megamind/library/handlers/utils
    alice/megamind/library/models/cards
    alice/megamind/library/testing

    alice/megamind/protos/quality_storage

    alice/library/proto

    library/cpp/testing/gmock_in_unittest
    library/cpp/testing/unittest
)

SRCS(
    http_response_ut.cpp
    logs_util_ut.cpp
    sensors_ut.cpp
)

REQUIREMENTS(ram:12)

END()
