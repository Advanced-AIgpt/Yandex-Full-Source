LIBRARY()

OWNER(
    yagafarov
    g:megamind
)

PEERDIR(
    alice/hollywood/library/modifiers/base_modifier
    alice/hollywood/library/modifiers/matchers
    alice/hollywood/library/modifiers/registry
)

SRCS(
    GLOBAL register.cpp
    voice_doodle_modifier.cpp
)

END()

