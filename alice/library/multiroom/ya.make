LIBRARY()

OWNER(g:alice sparkle igor-darov)

PEERDIR(
    alice/megamind/protos/scenarios
    alice/protos/data/device
)

SRCS(
    location_info.cpp
)

END()

RECURSE_FOR_TESTS(ut)
