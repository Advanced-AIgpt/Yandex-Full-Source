LIBRARY()

OWNER(g:megamind)

SRCS(
    dialog_history.cpp
    session.cpp
)

PEERDIR(
    alice/megamind/library/scenarios/defs
    alice/megamind/library/session/protos
    alice/megamind/library/stack_engine/protos
    alice/megamind/library/util
    alice/megamind/protos/common
    alice/megamind/protos/modifiers
    alice/megamind/protos/proactivity
    alice/megamind/protos/scenarios
    library/cpp/json
    library/cpp/protobuf/json
    library/cpp/string_utils/base64
)

END()

RECURSE_FOR_TESTS(
    ut
)
