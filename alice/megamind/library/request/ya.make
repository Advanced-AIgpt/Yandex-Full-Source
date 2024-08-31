LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/client
    alice/library/experiments
    alice/library/geo
    alice/library/music
    alice/library/restriction_level
    alice/library/unittest

    alice/megamind/library/biometry
    alice/megamind/library/common
    alice/megamind/library/models/directives
    alice/megamind/library/request/internal/protos
    alice/megamind/library/scenarios/defs
    alice/megamind/library/speechkit
    alice/megamind/library/stack_engine/protos
    alice/megamind/library/util
    alice/megamind/protos/scenarios

    alice/memento/proto
    alice/protos/data
)

SRCS(
    builder.cpp
    request.cpp
)

GENERATE_ENUM_SERIALIZATION(request.h)

END()

RECURSE(
    event
    internal
)

RECURSE_FOR_TESTS(ut)
