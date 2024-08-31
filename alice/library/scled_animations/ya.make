LIBRARY()

OWNER(d-dima)

PEERDIR(
    alice/hollywood/library/response
    alice/megamind/protos/scenarios
)

SRCS(
    scled_animations_builder.cpp
    scled_animations_directive.cpp
    scled_animations_directive_hw.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)