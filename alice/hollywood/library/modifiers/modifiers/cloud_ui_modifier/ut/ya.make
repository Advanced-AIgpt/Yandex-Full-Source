UNITTEST_FOR(alice/hollywood/library/modifiers/modifiers/cloud_ui_modifier)

OWNER(
    sparkle
    g:hollywood
)

PEERDIR(
    library/cpp/testing/common
    library/cpp/testing/gmock_in_unittest
    library/cpp/testing/unittest

    apphost/lib/service_testing
)

SRCS(
    cloud_ui_modifier_ut.cpp
)

END()
