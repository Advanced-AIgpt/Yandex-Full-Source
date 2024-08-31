LIBRARY()

OWNER(
    g:megamind
)

PEERDIR(
    alice/hollywood/library/modifiers/base_modifier
    alice/hollywood/library/modifiers/registry

    alice/hollywood/library/modifiers/modifiers/conjugator_modifier/proto
    alice/megamind/protos/analytics/modifiers/conjugator
    alice/protos/api/conjugator
    alice/protos/data/language
)

SRCS(
    GLOBAL register.cpp
    conjugator_modifier.cpp
    conjugatable_scenarios_matcher.cpp
    layout_inspector.cpp
)

END()

RECURSE(
    proto
    ut
)
