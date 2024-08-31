LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/megamind/library/globalctx
    alice/megamind/library/registry

    infra/udp_click_metrics/client

    library/cpp/neh
)

SRCS(
    handler.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
