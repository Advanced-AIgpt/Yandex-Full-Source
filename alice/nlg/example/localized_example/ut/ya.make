UNITTEST_FOR(
    alice/nlg/example/localized_example
)

OWNER(g:alice)

SRCS(
    test_localized_example.cpp
)

PEERDIR(
    alice/library/util
    alice/nlg/library/nlg_renderer
    library/cpp/testing/common
    library/cpp/testing/unittest
)

DATA(arcadia/alice/nlg/example/localized_example/translations.json)

END()
