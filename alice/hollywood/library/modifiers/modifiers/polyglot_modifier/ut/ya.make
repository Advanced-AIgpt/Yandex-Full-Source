UNITTEST_FOR(alice/hollywood/library/modifiers/modifiers/polyglot_modifier)

OWNER(
    alexanderplat
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
    output_speech_modifier_ut.cpp
    polyglot_modifier_ut.cpp
)

END()
