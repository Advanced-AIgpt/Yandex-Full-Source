LIBRARY()

OWNER(
    sparkle
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/registry
    alice/hollywood/library/response
)

SRCS(
    GLOBAL repeat_after_me.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
