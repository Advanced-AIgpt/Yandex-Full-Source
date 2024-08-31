LIBRARY()

OWNER(
    flimsywhimsy
    g:alice
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/frame
    alice/hollywood/library/global_context
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/link_a_remote/nlg
    alice/library/proto
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
)

SRCS(
    GLOBAL link_a_remote.cpp
)

END()
