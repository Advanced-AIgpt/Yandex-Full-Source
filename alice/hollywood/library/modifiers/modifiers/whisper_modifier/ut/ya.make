UNITTEST_FOR(alice/hollywood/library/modifiers/modifiers/whisper_modifier)

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
)

SRCS(
    whisper_modifier_ut.cpp
)

END()
