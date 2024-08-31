LIBRARY()

OWNER(g:megamind)

SRCS(
    scenario_ref.cpp
    scenario_wrapper.cpp
)

PEERDIR(
    alice/megamind/library/apphost_request/protos
    alice/megamind/library/scenarios/interface
    alice/megamind/library/session
    alice/megamind/library/util
    alice/megamind/protos/common
    alice/megamind/protos/modifiers
)

END()
