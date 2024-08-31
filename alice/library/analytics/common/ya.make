LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/megamind/protos/scenarios
    library/cpp/cgiparam
    library/cpp/json
)

SRCS(
    names.cpp
    product_scenarios.cpp
    utils.cpp
)

END()

RECURSE_FOR_TESTS(ut)
