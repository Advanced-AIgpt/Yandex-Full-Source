LIBRARY()

OWNER(
    tolyandex
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/megamind/protos/scenarios
    alice/megamind/protos/common
    alice/library/json
)

SRCS(
    debug_md.cpp
    serp_helpers.cpp
    utils.cpp
)

END()
