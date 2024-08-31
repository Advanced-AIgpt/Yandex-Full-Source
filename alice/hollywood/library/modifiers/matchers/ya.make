LIBRARY()

OWNER(
    yagafarov
    g:megamind
)

PEERDIR(
    alice/hollywood/library/modifiers/internal/config/proto
    alice/hollywood/library/modifiers/util
)

SRCS(
    exact_matcher.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
