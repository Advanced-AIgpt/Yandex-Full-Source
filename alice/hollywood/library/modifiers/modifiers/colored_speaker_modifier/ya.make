LIBRARY()

OWNER(
    yagafarov
    g:megamind
)

PEERDIR(
    alice/hollywood/library/modifiers/base_modifier
    alice/hollywood/library/modifiers/internal/config/proto
    alice/hollywood/library/modifiers/matchers
    alice/hollywood/library/modifiers/registry

    alice/library/proto
    alice/library/util
)

SRCS(
    colored_speaker_modifier.cpp
    GLOBAL register.cpp
)

END()


RECURSE_FOR_TESTS(
    ut
)
