UNITTEST_FOR(alice/hollywood/library/modifiers/modifiers/conjugator_modifier)

OWNER(
    g:hollywood
)

PEERDIR(
    library/cpp/testing/common
    library/cpp/testing/gmock_in_unittest
    library/cpp/testing/unittest

    alice/library/unittest

    apphost/lib/service_testing
)

SRCS(
    conjugatable_scenarios_matcher_ut.cpp
    conjugator_modifier_ut.cpp
    layout_inspector_ut.cpp
)

END()
