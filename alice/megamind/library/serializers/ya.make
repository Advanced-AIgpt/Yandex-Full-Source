LIBRARY()

OWNER(
    alkapov
    g:megamind
)

PEERDIR(
    alice/library/json
    alice/library/logger
    alice/library/multiroom
    alice/library/search
    alice/library/typed_frame
    alice/megamind/library/common
    alice/megamind/library/models/buttons
    alice/megamind/library/models/cards
    alice/megamind/library/models/directives
    alice/megamind/library/models/interfaces
    alice/megamind/library/scenarios/defs
    alice/megamind/protos/scenarios
    alice/megamind/protos/speechkit
    alice/protos/data
    alice/protos/div
    alice/protos/extensions
    library/cpp/cgiparam
    library/cpp/json
)

SRCS(
    meta.cpp
    scenario_proto_deserializer.cpp
    scenario_proto_serializer.cpp
    speechkit_proto_serializer.cpp
    speechkit_struct_serializer.cpp
)

END()

RECURSE_FOR_TESTS(ut)
