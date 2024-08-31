LIBRARY()

OWNER(
    sparkle
    g:megamind
)

PEERDIR(
    alice/hollywood/library/modifiers/base_modifier
    alice/hollywood/library/modifiers/registry
)

SRCS(
    cloud_ui_modifier.cpp
    GLOBAL register.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
