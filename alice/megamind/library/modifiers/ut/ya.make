UNITTEST_FOR(alice/megamind/library/modifiers)

OWNER(
    jan-fazli
    g:megamind
)

PEERDIR(
    alice/library/geo
    alice/megamind/library/context
    alice/megamind/library/kv_saas
    alice/megamind/library/response
    alice/megamind/library/session
    alice/megamind/library/speechkit
    alice/megamind/library/testing
    alice/megamind/library/util
    alice/megamind/protos/modifiers
    library/cpp/testing/gmock_in_unittest
    library/cpp/timezone_conversion
)

SRCS(
    modifier_ut.cpp
)

END()
