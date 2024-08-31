LIBRARY()

OWNER(
    jan-fazli
    olegator
    g:megamind
)

PEERDIR(
    alice/library/experiments
    alice/library/frame
    alice/library/proactivity/apply_conditions
    alice/library/proactivity/success_conditions
    alice/library/proto_eval
    alice/megamind/library/analytics
    alice/megamind/library/experiments
    alice/megamind/library/modifiers
    alice/megamind/library/proactivity/common
    alice/megamind/library/scenarios/defs
    alice/megamind/library/vins
    alice/megamind/protos/modifiers
    alice/megamind/protos/proactivity
    alice/megamind/protos/scenarios
    alice/memento/proto
    contrib/libs/re2
    dj/services/alisa_skills/profile/proto
    library/cpp/expression
    library/cpp/resource
)

SRCS(
    postroll.cpp
    postroll_actions.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
