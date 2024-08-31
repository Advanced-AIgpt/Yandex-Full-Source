LIBRARY()

OWNER(
    yagafarov
    g:megamind
)

PEERDIR(
    alice/hollywood/library/modifiers/base_modifier
    alice/hollywood/library/modifiers/external_sources
    alice/hollywood/library/modifiers/context
    library/cpp/testing/gmock_in_unittest
)

SRCS(
    fake_modifier.cpp
    mock_external_source_request_collector.cpp
    mock_modifier_context.cpp
)

END()
