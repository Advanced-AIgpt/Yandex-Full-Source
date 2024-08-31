LIBRARY()

OWNER(
    yagafarov
    g:megamind
)

PEERDIR(
    alice/hollywood/library/modifiers/base_modifier
    alice/hollywood/library/modifiers/registry
)

SRCS(
    whisper_modifier.cpp
    GLOBAL register.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)

