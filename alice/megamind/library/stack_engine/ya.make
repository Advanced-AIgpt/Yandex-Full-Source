LIBRARY()

OWNER(
    g:megamind
    alkapov
)

PEERDIR(
    alice/megamind/library/stack_engine/protos
)

SRCS(
    stack_engine.cpp
)

END()

RECURSE(
    protos
)

RECURSE_FOR_TESTS(ut)
