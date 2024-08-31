LIBRARY()

OWNER(
    g:alice
)

PEERDIR(
    alice/megamind/protos/common
)

SRCS(
    experiments.cpp
    flags.cpp
    utils.cpp
)

END()

RECURSE_FOR_TESTS(ut)
