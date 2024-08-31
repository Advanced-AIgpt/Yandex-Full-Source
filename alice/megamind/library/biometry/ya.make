LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/megamind/protos/scenarios
)

SRCS(
    biometry.cpp
)

END()

RECURSE_FOR_TESTS(ut)
