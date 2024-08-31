LIBRARY()

OWNER(
    yagafarov
    g:megamind
)

PEERDIR(
    alice/hollywood/library/modifiers/analytics_info
    alice/hollywood/library/modifiers/context
    alice/hollywood/library/modifiers/external_sources
    alice/hollywood/library/modifiers/response_body_builder

    alice/megamind/protos/modifiers
)

SRCS(
    base_modifier.cpp
)

GENERATE_ENUM_SERIALIZATION(base_modifier.h)

END()

RECURSE_FOR_TESTS(
    ut
)
