LIBRARY()

OWNER(g:megamind)

SRCS(
    data_sources.cpp
)

PEERDIR(
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
)

END()

RECURSE_FOR_TESTS(
    ut
)
