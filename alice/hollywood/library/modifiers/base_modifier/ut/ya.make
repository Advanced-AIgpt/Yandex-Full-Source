UNITTEST_FOR(alice/hollywood/library/modifiers/base_modifier)

OWNER(
    yagafarov
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/modifiers/testing

    alice/library/unittest

    library/cpp/testing/common
    library/cpp/testing/gmock_in_unittest
    library/cpp/testing/unittest
)

SRCS(
    base_modifier_ut.cpp
)

END()
