LIBRARY()

OWNER(
    jan-fazli
    olegator
    g:megamind
)

PEERDIR(
    alice/library/geo
    alice/library/json
    alice/library/response
    alice/megamind/library/analytics
    alice/megamind/library/context
    alice/megamind/library/kv_saas
    alice/megamind/library/models/directives
    alice/megamind/library/response
    alice/megamind/library/session
    alice/megamind/library/speechkit
    alice/megamind/library/util
    alice/megamind/protos/modifiers
    alice/megamind/protos/proactivity
    alice/memento/proto
    dj/services/alisa_skills/server/proto/client
    library/cpp/timezone_conversion
)

SRCS(
    context.cpp
    modifier.cpp
    utils.cpp
)

GENERATE_ENUM_SERIALIZATION(modifier.h)

END()

RECURSE_FOR_TESTS(ut)
