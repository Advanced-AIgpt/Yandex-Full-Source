UNITTEST_FOR(alice/megamind/library/globalctx/impl)

OWNER(
    g:megamind
)

SIZE(MEDIUM)

SRCS(
    globalctx_ut.cpp
)

PEERDIR(
    alice/library/unittest
    alice/megamind/library/globalctx
    alice/megamind/library/testing
    library/cpp/testing/unittest
)

END()
