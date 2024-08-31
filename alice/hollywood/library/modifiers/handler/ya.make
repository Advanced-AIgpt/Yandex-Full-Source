LIBRARY()

OWNER(
    yagafarov
    g:megamind
)

PEERDIR(
    alice/hollywood/library/modifiers/analytics_info
    alice/hollywood/library/modifiers/base_modifier
    alice/hollywood/library/modifiers/metrics
    alice/hollywood/library/modifiers/registry
    alice/hollywood/library/modifiers/response_body_builder

    alice/hollywood/library/base_hw_service
    alice/hollywood/library/registry
    alice/hollywood/library/util

    alice/library/util
    alice/megamind/protos/modifiers
)

SRCS(
    modifier_apply_handle.cpp
    modifier_prepare_handle.cpp
    GLOBAL register.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
