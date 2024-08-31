LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/json
    alice/library/network
    alice/megamind/library/request_composite
    alice/megamind/library/sources
    alice/megamind/library/util
    alice/megamind/protos/common
    library/cpp/json
)

SRCS(
    misspell.cpp
    utils.cpp
)

END()

RECURSE_FOR_TESTS(ut)
