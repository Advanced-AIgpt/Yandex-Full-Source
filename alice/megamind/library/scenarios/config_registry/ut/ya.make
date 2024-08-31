UNITTEST_FOR(alice/megamind/library/scenarios/config_registry)

OWNER(g:megamind)

PEERDIR(
    alice/megamind/library/config
    library/cpp/testing/unittest
)

SRCS(
    combinator_config_registry_ut.cpp
    config_registry_ut.cpp
)

DATA(
    arcadia/alice/megamind/library/scenarios/config_registry/ut/scenarios
)

END()
