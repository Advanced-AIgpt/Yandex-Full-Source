LIBRARY()

OWNER(
    lyalchenko
)

SRCS(
    machines.cpp
    states.cpp
    events.cpp
)

PEERDIR(
    alice/cuttlefish/library/digest
)

END()


RECURSE_FOR_TESTS(
    ut
)
