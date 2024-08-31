UNITTEST_FOR(alice/megamind/library/new_modifiers)

OWNER(
    yagafarov
    g:megamind
)

PEERDIR(
    alice/library/json
    alice/megamind/library/testing
    library/cpp/testing/unittest
)

SRCS(
    modifier_request_factory_ut.cpp
    utils_ut.cpp
)

REQUIREMENTS(ram:9)

END()
