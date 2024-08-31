UNITTEST_FOR(alice/megamind/library/stage_wrappers)

OWNER(
    g:megamind
)

PEERDIR(
    alice/megamind/library/apphost_request/protos
    alice/megamind/library/testing

    alice/library/unittest

    library/cpp/testing/common
    library/cpp/testing/gmock_in_unittest
    library/cpp/testing/unittest
)

SRCS(
    postclassify_state_ut.cpp
)

REQUIREMENTS(ram:9)

END()
