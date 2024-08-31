LIBRARY()

OWNER(g:megamind)

SRCS(
    registry.cpp
)

PEERDIR(
    alice/megamind/library/scenarios/helpers
    alice/megamind/library/scenarios/protocol
    alice/megamind/library/scenarios/quasar
    alice/megamind/library/scenarios/registry/interface
)

END()
