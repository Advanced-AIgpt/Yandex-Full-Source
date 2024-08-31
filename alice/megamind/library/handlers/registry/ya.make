LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/megamind/library/handlers/apphost_megamind
    alice/megamind/library/handlers/apphost_utility
    alice/megamind/library/handlers/speechkit
    alice/megamind/library/registry
    alice/megamind/library/util
    infra/udp_click_metrics/client
)

SRCS(
    construct.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
